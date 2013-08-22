#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "utils.h"
#include "kernobjc/runtime.h"
#include "sarray2.h"
#include "dtable.h"
#include "selector.h"
#include "slot_pool.h"
#include "runtime.h"
#include "class.h"
#include "class_registry.h"
#include "message.h"
#include "private.h"
#include "init.h"
#include "exception.h"

PRIVATE dtable_t uninstalled_dtable;

/* Head of the list of temporary dtables.  Protected by initialize_lock. */
PRIVATE InitializingDtable *temporary_dtables;
/* Lock used to protect the temporary dtables list. */
PRIVATE objc_rw_lock initialize_lock;

/*
 * Returns YES if the class implements a method for the specified selector, NO
 * otherwise.
 */
static BOOL ownsMethod(Class cls, SEL sel)
{
	struct objc_slot *slot = objc_get_slot(cls, sel);
	if ((NULL != slot) && (slot->owner == cls))
	{
		return YES;
	}
	return NO;
}

static inline BOOL _objc_check_class_for_custom_arr_method(Class cls, SEL sel){
	struct objc_slot *slot = objc_get_slot(cls, sel);
	if (NULL != slot){
		cls->flags.has_custom_arr = YES;
		return YES;
	}
	return NO;
}

/*
 * Checks whether the class implements memory management methods, and whether
 * they are safe to use with ARC.
 */
static void checkARCAccessors(Class cls)
{
	if (!ownsMethod(cls, objc_is_arc_compatible_selector)){
		/*
		 * The class doesn't implement the isARC selector, which means 
		 * we need to check if it implements custom ARR methods.
		 */
		if (_objc_check_class_for_custom_arr_method(cls, objc_retain_selector)){
			return;
		}
		if (_objc_check_class_for_custom_arr_method(cls, objc_release_selector)){
			return;
		}
		if (_objc_check_class_for_custom_arr_method(cls, objc_autorelease_selector)){
			return;
		}
	}
	cls->flags.has_custom_arr = NO;
}

static void collectMethodsForMethodListToSparseArray(
		objc_method_list *list,
		SparseArray *sarray,
		BOOL recurse)
{
	if (recurse && (NULL != list->next))
	{
		collectMethodsForMethodListToSparseArray(list->next, sarray, YES);
	}
	for (unsigned i=0 ; i<list->size ; i++)
	{
		SparseArrayInsert(sarray, list->list[i].selector,
				(void*)&list->list[i]);
	}
}


PRIVATE void
objc_dispatch_tables_init(void)
{
	objc_debug_log("Initializing dispatch tables.\n");
	
	objc_rw_lock_init(&initialize_lock, "objc_initialize_lock");
	uninstalled_dtable = SparseArrayNew();
}

PRIVATE void
objc_dispatch_tables_destroy(void)
{
	objc_debug_log("Destroying dispatch tables.\n");
	
	objc_rw_lock_destroy(&initialize_lock);
	SparseArrayDestroy(&uninstalled_dtable);
}

static BOOL installMethodInDtable(Class class,
                                  Class owner,
                                  SparseArray *dtable,
                                  struct objc_method *method,
                                  BOOL replaceExisting)
{
	objc_debug_log("Installing method %s into dtable of class %s\n", sel_getName(method->selector), class_getName(class));
	
	objc_assert(uninstalled_dtable != dtable, "");
	SEL sel_id = method->selector;
	struct objc_slot *slot = SparseArrayLookup(dtable, sel_id);
	if (NULL != slot)
	{
		// If this method is the one already installed, pretend to install it again.
		if (slot->implementation == method->implementation) { return NO; }

		// If the existing slot is for this class, we can just replace the
		// implementation.  We don't need to bump the version; this operation
		// updates cached slots, it doesn't invalidate them.  
		if (slot->owner == owner)
		{
			// Don't replace methods if we're not meant to (if they're from
			// later in a method list, for example)
			if (!replaceExisting) { return NO; }
			slot->implementation = method->implementation;
			return YES;
		}

		// Check whether the owner of this method is a subclass of the one that
		// owns this method.  If it is, then we don't want to install this
		// method irrespective of other cases, because it has been overridden.
		for (struct objc_class *installedFor = slot->owner ;
				Nil != installedFor ;
				installedFor = installedFor->super_class)
		{
			
			if (installedFor == owner)
			{
				return NO;
			}
		}
	}
	struct objc_slot *oldSlot = slot;
	slot = objc_slot_create_for_method_in_class((void*)method, owner);
	SparseArrayInsert(dtable, sel_id, slot);
	
	// Invalidate the old slot, if there is one.
	if (NULL != oldSlot)
	{
		oldSlot->version++;
	}
	return YES;
}

static void installMethodsInClass(Class cls,
                                  Class owner,
                                  SparseArray *methods,
                                  BOOL replaceExisting)
{
	SparseArray *dtable = dtable_for_class(cls);
	objc_assert(uninstalled_dtable != dtable, "");

	uint16_t idx = 0;
	struct objc_method *m;
	while ((m = SparseArrayNext(methods, &idx)))
	{
		if (!installMethodInDtable(cls, owner, dtable, m, replaceExisting))
		{
			// Remove this method from the list, if it wasn't actually installed
			SparseArrayInsert(methods, idx, 0);
		}
	}
}

static void mergeMethodsFromSuperclass(Class super, Class cls, SparseArray *methods)
{
	for (struct objc_class *subclass=cls->subclass_list ; 
		Nil != subclass ; subclass = subclass->sibling_list)
	{
		// Don't bother updating dtables for subclasses that haven't been
		// initialized yet
		if (!classHasDtable(subclass)) { continue; }

		// Create a new (copy-on-write) array to pass down to children
		SparseArray *newMethods = SparseArrayCopy(methods);
		// Install all of these methods except ones that are overridden in the
		// subclass.  All of the methods that we are updating were added in a
		// superclass, so we don't replace versions registered to the subclass.
		installMethodsInClass(subclass, super, newMethods, YES);
		// Recursively add the methods to the subclass's subclasses.
		mergeMethodsFromSuperclass(super, subclass, newMethods);
		SparseArrayDestroy(&newMethods);
	}
}

static inline void _add_method_list_to_class(Class cls, objc_method_list *list,
					     BOOL follow){
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	SparseArray *methods = SparseArrayNew();
	collectMethodsForMethodListToSparseArray(list, methods, follow);
	installMethodsInClass(cls, cls, methods, YES);
	// Methods now contains only the new methods for this class.
	mergeMethodsFromSuperclass(cls, cls, methods);
	SparseArrayDestroy(&methods);
	checkARCAccessors(cls);
}


PRIVATE void dtable_add_method_list_to_class(Class cls,
                                      objc_method_list *list)
{
	// Only update real dtables
	if (!classHasDtable(cls) || list == NULL) { return; }

	objc_debug_log("adding method lists to dtable for class %s\n", class_getName(cls));
	_add_method_list_to_class(cls, list, NO);
}


PRIVATE void objc_update_dtable_for_class(Class cls)
{
	
	// Only update real dtables and real classes
	if (!classHasDtable(cls)) { return; }
	
	objc_debug_log("updating dtable for class %s\n", class_getName(cls));
	
	_add_method_list_to_class(cls, cls->methods, YES);
}

static dtable_t create_dtable_for_class(Class class, dtable_t root_dtable)
{
	// Don't create a dtable for a class that already has one
	if (classHasDtable(class)) { return dtable_for_class(class); }

	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	objc_debug_log("creating dtable for class %s\n", class_getName(class));

	// Make sure that another thread didn't create the dtable while we were
	// waiting on the lock.
	if (classHasDtable(class)) { return dtable_for_class(class); }

	Class super = class_getSuperclass(class);
	dtable_t dtable;


	if (Nil == super)
	{
		dtable = SparseArrayNew();
	}
	else
	{
		dtable_t super_dtable = dtable_for_class(super);
		if (super_dtable == uninstalled_dtable)
		{
			if (super->isa == class)
			{
				super_dtable = root_dtable;
			}
			else
			{
				objc_abort("Creating a dtable for a class that isn't the class\n");
			}
		}
		dtable = SparseArrayCopy(super_dtable);
	}

	// When constructing the initial dtable for a class, we iterate along the
	// method list in forward-traversal order.  The first method that we
	// encounter is always the one that we want to keep, so we instruct
	// installMethodInDtable() not to replace methods that are already
	// associated with this class.
	objc_method_list *list = class->methods;

	while (NULL != list)
	{
		for (unsigned i=0 ; i<list->size ; i++)
		{
			installMethodInDtable(class, class, dtable, &list->list[i], NO);
		}
		list = list->next;
	}

	return dtable;
}

PRIVATE dtable_t objc_copy_dtable_for_class(dtable_t old, Class cls)
{
	return SparseArrayCopy(old);
}

PRIVATE void free_dtable(dtable_t *dtable)
{
	if (*dtable != uninstalled_dtable){
		SparseArrayDestroy(dtable);
	}
}

__attribute__((unused)) static void objc_release_object_lock(id *x)
{
	objc_sync_exit(*x);
}
/*
 * Macro that is equivalent to @synchronize, for use in C code.
 */
#define LOCK_OBJECT_FOR_SCOPE(obj) \
	__attribute__((cleanup(objc_release_object_lock)))\
	__attribute__((unused)) id lock_object_pointer = obj;\
	objc_sync_enter(obj);

/*
 * Remove a buffer from an entry in the initializing dtables list.  This is
 * called as a cleanup to ensure that it runs even if +initialize throws an
 * exception.
 */
static void remove_dtable(InitializingDtable* meta_buffer)
{
	OBJC_LOCK(&initialize_lock);
	InitializingDtable *buffer = meta_buffer->next;
	// Install the dtable:
	meta_buffer->class->dtable = meta_buffer->dtable;
	buffer->class->dtable = buffer->dtable;
	// Remove the look-aside buffer entry.
	if (temporary_dtables == meta_buffer)
	{
		temporary_dtables = buffer->next;
	}
	else
	{
		InitializingDtable *prev = temporary_dtables;
		while (prev->next->class != meta_buffer->class)
		{
			prev = prev->next;
		}
		prev->next = buffer->next;
	}
	OBJC_UNLOCK(&initialize_lock);
}

/*
 * Send a +initialize message to the receiver, if required.  
 */
PRIVATE void objc_send_initialize(id object)
{
	Class class = objc_object_get_class_inline(object);
	// If the first message is sent to an instance (weird, but possible and
	// likely for things like NSConstantString, make sure +initialize goes to
	// the class not the metaclass.  
	if (class->flags.meta)
	{
		class = (Class)object;
	}
	Class meta = class->isa;


	// Make sure that the class is resolved.
	objc_class_resolve(class);

	// Make sure that the superclass is initialized first.
	if (Nil != class->super_class)
	{
		objc_send_initialize((id)class->super_class);
	}

	// Superclass +initialize might possibly send a message to this class, in
	// which case this method would be called again.  See NSObject and
	// NSAutoreleasePool +initialize interaction in GNUstep.
	if (class->flags.initialized)
	{
		// We know that initialization has started because the flag is set.
		// Check that it's finished by grabbing the class lock.  This will be
		// released once the class has been fully initialized
		objc_sync_enter((id)meta);
		objc_sync_exit((id)meta);
		objc_assert(dtable_for_class(class) != uninstalled_dtable, "");
		return;
	}

	// Lock the runtime while we're creating dtables and before we acquire any
	// other locks.  This prevents a lock-order reversal when 
	// dtable_for_class is called from something holding the runtime lock while
	// we're still holding the initialize lock.  We should ensure that we never
	// acquire the runtime lock after acquiring the initialize lock.
	OBJC_LOCK_RUNTIME();
	
	objc_debug_log("sending initialize to class %s - meta[%p]\n", object_getClassName(object), meta);
	
	OBJC_LOCK_OBJECT_FOR_SCOPE((id)meta);
	OBJC_LOCK(&initialize_lock);
	if (class->flags.initialized)
	{
		OBJC_UNLOCK(&initialize_lock);
		return;
	}
	
	BOOL skipMeta = meta->flags.initialized;

	// Set the initialized flag on both this class and its metaclass, to make
	// sure that +initialize is only ever sent once.
	class->flags.initialized = YES;
	meta->flags.initialized = YES;

	dtable_t class_dtable = create_dtable_for_class(class, uninstalled_dtable);
	dtable_t dtable = skipMeta ? 0 : create_dtable_for_class(meta, class_dtable);
	// Now we've finished doing things that may acquire the runtime lock, so we
	// can hold onto the initialise lock to make anything doing
	// dtable_for_class block until we've finished updating temporary dtable
	// lists.
	// If another thread holds the runtime lock, it can now proceed until it
	// gets into a dtable_for_class call, and then block there waiting for us
	// to finish setting up the temporary dtable.
	OBJC_UNLOCK_RUNTIME();

	struct objc_slot *initializeSlot = skipMeta ? 0 :
			objc_dtable_lookup(dtable, objc_initialize_selector);

	// If there's no initialize method, then don't bother installing and
	// removing the initialize dtable, just install both dtables correctly now
	if (0 == initializeSlot
		|| ((class->flags.meta && (initializeSlot->owner != class))
			|| (!class->flags.meta && (initializeSlot->owner != class->isa))))
	{
		objc_debug_log("Not sending +initialize message to class %s since the"
					   " +initialize method is implemented on %s\n",
					   class->name, initializeSlot == NULL ? "null" :
					   initializeSlot->owner->name);
		if (!skipMeta)
		{
			meta->dtable = dtable;
		}
		class->dtable = class_dtable;
		checkARCAccessors(class);
		OBJC_UNLOCK(&initialize_lock);
		return;
	}



	// Create an entry in the dtable look-aside buffer for this.  When sending
	// a message to this class in future, the lookup function will check this
	// buffer if the receiver's dtable is not installed, and block if
	// attempting to send a message to this class.
	InitializingDtable buffer = { class, class_dtable, temporary_dtables };
	InitializingDtable meta_buffer = { meta, dtable, &buffer };
	temporary_dtables = &meta_buffer;
	// We now release the initialize lock.  We'll reacquire it later when we do
	// the cleanup, but at this point we allow other threads to get the
	// temporary dtable and call +initialize in other threads.
	OBJC_UNLOCK(&initialize_lock);
	// We still hold the class lock at this point.  dtable_for_class will block
	// there after acquiring the temporary dtable.

	checkARCAccessors(class);

	/* Since we don't have libunwind in the kernel yet, there may be some issues
	 * caused if the initialize method threw an exception -> the buffer needs
	 * to be removed from the temp dtables!
	 */
	struct objc_exception_handler handler;
	objc_exception_try_enter(&handler);
	if (setjmp(handler.jump_buffer) == 0){
		/* Try */
		// Store the buffer in the temporary dtables list.  Note that it is safe to
		// insert it into a global list, even though it's a temporary variable,
		// because we will clean it up after this function.
		initializeSlot->implementation((id)class, objc_initialize_selector);
		remove_dtable(&meta_buffer);
	}else{
		// Catch
		id _caught = objc_exception_extract(&handler);
		objc_exception_try_exit(&handler);
		remove_dtable(&meta_buffer);
		objc_exception_throw(_caught);
	}
}

PRIVATE void objc_install_dtable_for_object(id receiver){
	Class cl = objc_object_get_class_inline(receiver);
	
	objc_debug_log("Installing dtable on class %s%s%s.\n",
		       object_getClassName(receiver),
		       cl->flags.fake ? "[fake]" : "",
		       cl->flags.meta ? " (meta)" : "");
	
	dtable_t dtable = dtable_for_class(cl);
	/* Install the dtable if it hasn't already been initialized. */
	if (dtable == uninstalled_dtable){
		objc_send_initialize(receiver);
		dtable = dtable_for_class(cl);
	}
}



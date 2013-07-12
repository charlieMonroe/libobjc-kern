
#include "dtable.h"
#include "selector.h"
#include "slot_pool.h"
#include "runtime.h"
#include "class.h"
#include "class_registry.h"
#include "message.h"

PRIVATE dtable_t uninstalled_dtable;

/** Head of the list of temporary dtables.  Protected by initialize_lock. */
PRIVATE InitializingDtable *temporary_dtables;
/** Lock used to protect the temporary dtables list. */
PRIVATE objc_rw_lock initialize_lock;

/**
 * Returns YES if the class implements a method for the specified selector, NO
 * otherwise.
 */
static BOOL ownsMethod(Class cls, SEL sel)
{
	struct objc_slot *slot = objc_class_get_slot(cls, sel);
	if ((NULL != slot) && (slot->owner == cls))
	{
		return YES;
	}
	return NO;
}

/**
 * Checks whether the class implements memory management methods, and whether
 * they are safe to use with ARC.
 */
static void checkARCAccessors(Class cls)
{
	static SEL retain, release, autorelease, isARC;
	if (0 == retain)
	{
		retain = objc_selector_register("retain", "@@:");
		release = objc_selector_register("release", "v@:");
		autorelease = objc_selector_register("autorelease", "@@:");
		
		// TODO necessary?
		isARC = objc_selector_register("_ARCCompliantRetainRelease", "@@:");
	}
	struct objc_slot *slot = objc_class_get_slot(cls, retain);
	if ((0 != slot) && !ownsMethod(slot->owner, isARC))
	{
		cls->flags.has_custom_arr = YES;
		return;
	}
	slot = objc_class_get_slot(cls, release);
	if ((NULL != slot) && !ownsMethod(slot->owner, isARC))
	{
		cls->flags.has_custom_arr = YES;
		return;
	}
	slot = objc_class_get_slot(cls, autorelease);
	if ((NULL != slot) && !ownsMethod(slot->owner, isARC))
	{
		cls->flags.has_custom_arr = YES;
		return;
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
		SparseArrayInsert(sarray, list->method_list[i].selector,
				(void*)&list->method_list[i]);
	}
}


PRIVATE void init_dispatch_tables ()
{
	objc_rw_lock_init(&initialize_lock);
	uninstalled_dtable = SparseArrayNew();
}

static BOOL installMethodInDtable(Class class,
                                  Class owner,
                                  SparseArray *dtable,
                                  struct objc_method *method,
                                  BOOL replaceExisting)
{
	objc_assert(uninstalled_dtable != dtable, "");
	uint16_t sel_id = method->selector;
	struct objc_slot *slot = SparseArrayLookup(dtable, sel_id);
	if (NULL != slot)
	{
		// If this method is the one already installed, pretend to install it again.
		if (slot->method == method->implementation) { return NO; }

		// If the existing slot is for this class, we can just replace the
		// implementation.  We don't need to bump the version; this operation
		// updates cached slots, it doesn't invalidate them.  
		if (slot->owner == owner)
		{
			// Don't replace methods if we're not meant to (if they're from
			// later in a method list, for example)
			if (!replaceExisting) { return NO; }
			slot->method = method->implementation;
			return YES;
		}

		// Check whether the owner of this method is a subclass of the one that
		// owns this method.  If it is, then we don't want to install this
		// method irrespective of other cases, because it has been overridden.
		for (struct objc_class *installedFor = slot->owner ;
				Nil != installedFor ;
				installedFor = installedFor->super_class)
		{
			
			printf("Checking class %s, superclass ptr %p\n", installedFor->name, installedFor->super_class);
			if (installedFor == owner)
			{
				return NO;
			}
		}
	}
	struct objc_slot *oldSlot = slot;
	slot = new_slot_for_method_in_class((void*)method, owner);
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
		SparseArrayDestroy(newMethods);
	}
}

PRIVATE void objc_update_dtable_for_class(Class cls)
{
	// Only update real dtables
	if (!classHasDtable(cls)) { return; }

	OBJC_LOCK_RUNTIME_FOR_SCOPE();

	SparseArray *methods = SparseArrayNew();
	collectMethodsForMethodListToSparseArray((void*)cls->methods, methods, YES);
	installMethodsInClass(cls, cls, methods, YES);
	// Methods now contains only the new methods for this class.
	mergeMethodsFromSuperclass(cls, cls, methods);
	SparseArrayDestroy(methods);
	checkARCAccessors(cls);
}

PRIVATE void add_method_list_to_class(Class cls,
                                      objc_method_list *list)
{
	// Only update real dtables
	if (!classHasDtable(cls)) { return; }

	OBJC_LOCK_RUNTIME_FOR_SCOPE();

	SparseArray *methods = SparseArrayNew();
	collectMethodsForMethodListToSparseArray(list, methods, NO);
	installMethodsInClass(cls, cls, methods, YES);
	// Methods now contains only the new methods for this class.
	mergeMethodsFromSuperclass(cls, cls, methods);
	SparseArrayDestroy(methods);
	checkARCAccessors(cls);
}

static dtable_t create_dtable_for_class(Class class, dtable_t root_dtable)
{
	// Don't create a dtable for a class that already has one
	if (classHasDtable(class)) { return dtable_for_class(class); }

	OBJC_LOCK_RUNTIME_FOR_SCOPE();

	// Make sure that another thread didn't create the dtable while we were
	// waiting on the lock.
	if (classHasDtable(class)) { return dtable_for_class(class); }

	Class super = objc_class_get_superclass(class);
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
				abort();
			}
		}
		dtable = SparseArrayCopy(super_dtable);
	}

	// When constructing the initial dtable for a class, we iterate along the
	// method list in forward-traversal order.  The first method that we
	// encounter is always the one that we want to keep, so we instruct
	// installMethodInDtable() not to replace methods that are already
	// associated with this class.
	objc_method_list *list = (void*)class->methods;

	while (NULL != list)
	{
		for (unsigned i=0 ; i<list->size ; i++)
		{
			installMethodInDtable(class, class, dtable, &list->method_list[i], NO);
		}
		list = list->next;
	}

	return dtable;
}

PRIVATE dtable_t objc_copy_dtable_for_class(dtable_t old, Class cls)
{
	return SparseArrayCopy(old);
}

PRIVATE void free_dtable(dtable_t dtable)
{
	SparseArrayDestroy(dtable);
}

__attribute__((unused)) static void objc_release_object_lock(id *x)
{
	objc_sync_exit(*x);
}
/**
 * Macro that is equivalent to @synchronize, for use in C code.
 */
#define LOCK_OBJECT_FOR_SCOPE(obj) \
	__attribute__((cleanup(objc_release_object_lock)))\
	__attribute__((unused)) id lock_object_pointer = obj;\
	objc_sync_enter(obj);

/**
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

/**
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
	objc_resolve_class(class);

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

	static SEL initializeSel = 0;
	if (0 == initializeSel)
	{
		initializeSel = objc_selector_register("initialize", "v@:");
	}

	struct objc_slot *initializeSlot = skipMeta ? 0 :
			objc_dtable_lookup(dtable, initializeSel);

	// If there's no initialize method, then don't bother installing and
	// removing the initialize dtable, just install both dtables correctly now
	if (0 == initializeSlot)
	{
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
	__attribute__((cleanup(remove_dtable)))
	InitializingDtable meta_buffer = { meta, dtable, &buffer };
	temporary_dtables = &meta_buffer;
	// We now release the initialize lock.  We'll reacquire it later when we do
	// the cleanup, but at this point we allow other threads to get the
	// temporary dtable and call +initialize in other threads.
	OBJC_UNLOCK(&initialize_lock);
	// We still hold the class lock at this point.  dtable_for_class will block
	// there after acquiring the temporary dtable.

	checkARCAccessors(class);

	// Store the buffer in the temporary dtables list.  Note that it is safe to
	// insert it into a global list, even though it's a temporary variable,
	// because we will clean it up after this function.
	initializeSlot->method((id)class, initializeSel);
}


#include "os.h"
#include "kernobjc/runtime.h"
#include "types.h"
#include "runtime.h"
#include "utils.h"
#include "selector.h"
#include "sarray2.h"
#include "dtable.h"
#include "class_registry.h"
#include "class.h"
#include "private.h"
#include "init.h"
#include "class_extra.h"

/*
 * The initial capacities for the hash tables.
 */
#define OBJC_CLASS_TABLE_INITIAL_CAPACITY 512
#define OBJC_LOAD_TABLE_INITIAL_CAPACITY 512

#define MAP_TABLE_NAME objc_load_messages
#define MAP_TABLE_COMPARE_FUNCTION objc_pointers_are_equal
#define MAP_TABLE_HASH_KEY objc_hash_pointer
#define MAP_TABLE_HASH_VALUE objc_hash_pointer
#define MAP_TABLE_VALUE_TYPE IMP
#define MAP_MALLOC_TYPE M_LOAD_MSG_MAP_TYPE
#include "hashtable.h"

/*
 * A hash table with the load messages.
 */
static objc_load_messages_table *objc_load_messages;


/*
 * Calls a the load imp on a class if it hasn't been called already.
 */
static inline void
_objc_handle_load_imp_for_class(IMP load_imp, Class cl)
{
	/*
	 * The IMP mustn't be in the hash table. We hash the already called
	 * IMPs so that they don't get called twice.
	 */
	if (objc_load_messages_table_get(objc_load_messages, load_imp) == NULL){
		objc_debug_log("Will be sending a load message to class %s\n",
					   class_getName(cl));
		load_imp((id)cl, objc_load_selector);
		objc_load_messages_insert(objc_load_messages, load_imp);
		
		/* In the kernel, we force-initialize any class that implements +load.
		 * Implementing +load really makes a statement that this class is going
		 * to be used early, or later on, so it's better do so right now.
		 */
		objc_send_initialize((id)cl);
	}
}

PRIVATE void
objc_class_send_load_messages(Class cl)
{
	/* It's a class method, need to link at the meta class */
	Class meta = cl->isa;
	objc_method_list *list = meta->methods;
	while (list != NULL) {
		for (int i = 0; i < list->size; ++i){
			Method m = &list->list[i];
			if (m->selector == objc_load_selector){
				IMP load_imp = m->implementation;
				_objc_handle_load_imp_for_class(load_imp, cl);
			}
		}
		list = list->next;
	}
}


/* Forward declarations */
static inline BOOL _objc_class_name_is_equal_to(void *key, Class cl);
static inline uint32_t _objc_class_hash(Class cl);
static void _objc_class_fixup_instance_size(Class cl);

#define MAP_TABLE_NAME objc_class
#define MAP_TABLE_COMPARE_FUNCTION _objc_class_name_is_equal_to
#define MAP_TABLE_HASH_KEY objc_hash_string
#define MAP_TABLE_HASH_VALUE _objc_class_hash
#define MAP_TABLE_VALUE_TYPE Class
#define MAP_TABLE_NO_LOCK 1
#define MAP_MALLOC_TYPE M_CLASS_MAP_TYPE
#include "hashtable.h"

/*
 * A hash map with the classes.
 */
objc_class_table *objc_classes;

/*
 * The class tree leverages the fact that each class
 * has a sibling and child pointer in the structure,
 * pointing to an arbitrary child, or sibling respectively.
 *
 * This class_tree points to one of the root classes,
 * whichever gets registered first, and the rest of the
 * root classes is appended via the sibling pointer.
 *
 * The class doesn't get added to the tree until it is
 * finished (or loaded via loader).
 */
static Class class_tree;

/*
 * A pointer to the first unresolved class. The rest of
 * the unresolved classes are a linked list using
 * the unresolved pointer in the Class structure.
 */
static Class unresolved_classes;

/*
 * The runtime lock used for locking when modifying the
 * class hierarchy or modifying the classes in general.
 */
objc_rw_lock objc_runtime_lock;

/*
 * Default class lookup hook.
 */
static Class __objc_class_lookup_default_hook(const char *name) { return Nil; }

/*
 * The basic class lookup hook allowing the app to supply a class.
 */
Class(*objc_class_lookup_hook)(const char *name) =
__objc_class_lookup_default_hook;

/*
 * Default load callback.
 */
static void __objc_class_default_load_callback(Class cl) { }

/*
 * Whenever a class gets loaded, this function gets called.
 */
void(*objc_class_load_callback)(Class cl) = __objc_class_default_load_callback;


static inline BOOL
_objc_class_name_is_equal_to(void *key, Class cl)
{
	return objc_strings_equal(cl->name, (const char*)key);
}

static inline uint32_t
_objc_class_hash(Class cl)
{
	/* Just hash the name */
	return objc_hash_string(cl->name);
}

/*
 * Inserts cl into the siblings linked list
 */
static inline void
_objc_insert_class_to_back_of_sibling_list(Class cl, Class sibling)
{
	/* Inserting into the linked list */
	Class last_sibling = sibling->sibling_list;
	if (last_sibling == Nil){
		objc_log("Adding class %s to the sibling list (%s)\n", class_getName(cl),
					   class_getName(sibling));
		sibling->sibling_list = cl;
		sibling->isa->sibling_list = cl->isa;
		return;
	}
	
	while (last_sibling->sibling_list != Nil){
		last_sibling = last_sibling->sibling_list;
	}
	
	objc_log("Adding class %s to the sibling list (%s)\n", class_getName(cl),
				   class_getName(last_sibling));
	
	/* Add it to the end of the list */
	last_sibling->sibling_list = cl;
	/* And for the meta classes as well */
	last_sibling->isa->sibling_list = cl->isa;
}

/*
 * Inserts a class into the class tree. Note that the run-time
 * lock must be held.
 */
static inline void
_objc_insert_class_into_class_tree(Class cl)
{
	objc_log("Adding class %s to class tree\n", class_getName(cl));
	
	if (cl->subclass_list != Nil || cl->sibling_list != Nil){
		/* Already there. */
		return;
	}
	
	/* Insert the class into the tree. */
	if (cl->super_class == Nil){
		/* Root class */
		if (class_tree == Nil){
			/* The first one, yay! */
			objc_log("Adding class %s to to the class tree (root)\n", class_getName(cl));
			class_tree = cl;
		}else{
			_objc_insert_class_to_back_of_sibling_list(cl,
													   class_tree);
		}
	}else{
		/*
		 * It is a subclass, so need to insert it into the
		 * subclass list of the superclass as well as into
		 * the siblings list of the subclasses.
		 */
		Class super_class = cl->super_class;
		if (super_class->subclass_list == Nil){
			/* First subclass */
			super_class->subclass_list = cl;
			super_class->isa->subclass_list = cl->isa;
			
			objc_log("Adding class %s to class tree (first subcls of %s)\n",
					 class_getName(cl), class_getName(super_class));
		}else{
			_objc_insert_class_to_back_of_sibling_list(cl,
													   super_class->subclass_list);
		}
	}
}

/*
 * Removes a class from the class tree. Note that the run-time
 * lock must be held.
 */
static inline void
_objc_class_remove_from_class_tree(Class cl)
{
	Class *subclasses_ptr;
	if (cl->super_class == Nil){
		/* One of the root classes */
		subclasses_ptr = &class_tree;
	}else{
		subclasses_ptr = &cl->super_class->subclass_list;
	}
	
	if (*subclasses_ptr == cl){
		*subclasses_ptr = cl->sibling_list;
	}else{
		Class sibling = *subclasses_ptr;
		while (sibling != Nil) {
			if (sibling->sibling_list == cl){
				sibling->sibling_list = cl->sibling_list;
				
				cl->sibling_list = Nil;
				cl->subclass_list = Nil;
				return;
			}
			sibling = sibling->sibling_list;
		}
	}
	
	cl->sibling_list = Nil;
	cl->subclass_list = Nil;
}

/*
 * Removes the class from the unresolved list.
 */
static inline void
_objc_class_remove_from_unresolved_list(Class cl)
{
	if (cl->unresolved_class_next == Nil &&
	    cl->unresolved_class_previous == Nil &&
	    cl != unresolved_classes){
		/* Not on the list */
		return;
	}
	
	if (cl->unresolved_class_previous == Nil){
		/* The first class */
		objc_assert(cl == unresolved_classes, "There are possibly two "
					"unresolved class lists? (%p != %p)", cl,
					unresolved_classes);
		unresolved_classes = cl->unresolved_class_next;
	}else{
		cl->unresolved_class_previous->unresolved_class_next =
		cl->unresolved_class_next;
	}
	
	if (cl->unresolved_class_next != Nil){
		/* Not the end of the linked list */
		cl->unresolved_class_next->unresolved_class_previous =
		cl->unresolved_class_previous;
	}
	
	cl->unresolved_class_previous = Nil;
	cl->unresolved_class_next = Nil;
}

static unsigned int
_padding_for_ivar(Ivar ivar, size_t offset)
{
	return ((ivar->align - (offset % ivar->align)) % ivar->align);
}

static size_t
_objc_class_calculate_instance_size(Class cl)
{
	size_t size = 0;
	Class superclass = cl->super_class;
	if (superclass != Nil){
		if (superclass->instance_size == 0
		    && superclass->ivars != NULL
		    && superclass->ivars->size > 0){
			_objc_class_fixup_instance_size(superclass);
		}
		size = superclass->instance_size;
	}
	
	if (cl->ivars != NULL){
		for (int i = 0; i < cl->ivars->size; ++i){
			Ivar ivar = &cl->ivars->list[i];
			ivar->size = objc_sizeof_type(ivar->type);
			ivar->align = objc_alignof_type(ivar->type);
			
			size_t offset = size;
			if (size % ivar->align != 0){
				unsigned int padding = _padding_for_ivar(ivar,
														 offset);
				offset += padding;
			}
			ivar->offset = (int)offset;
			size = offset + ivar->size;
			
			/*
			 * The ->ivar_offsets can be generated by the compiler
			 * since the number of ivars in this class is known at
			 * compile time.
			 */
			cl->ivar_offsets[i] = offset;
			
		}
	}
	return size;
}

static void
_objc_class_fixup_instance_size(Class cl)
{
	if (cl->flags.fake || cl->flags.resolved){
		return;
	}
	
	objc_assert(cl != Nil, "Cannot fixup instance size of Nil class!\n");
	
	if (cl->flags.meta){
		/* Meta class has the size of the class structure */
		cl->instance_size = sizeof(struct objc_class);
	}else{
		cl->instance_size = _objc_class_calculate_instance_size(cl);
	}
	
/*	objc_debug_log("Fixing up instance size of class %s%s - %d bytes\n",
				   cl->name, cl->flags.meta ? " (meta)" : "",
				   (unsigned int)cl->instance_size);*/
}

static inline void
_objc_class_register_class_no_lock(Class cl){
	if (objc_class_table_get(objc_classes, cl->name) != Nil){
		objc_log("Class %s has been defined in multiple modules."
				 " Which one will be used is undefined.\n", cl->name);
		return;
	}
	
	objc_log("Registering class %s with the runtime.\n", cl->name);
	
	_objc_class_fixup_instance_size(cl);
	_objc_class_fixup_instance_size(cl->isa);
	
	objc_class_insert(objc_classes, cl);
	
	objc_register_selectors_from_class(cl, cl->isa);
	
	cl->dtable = uninstalled_dtable;
	cl->isa->dtable = uninstalled_dtable;
	
	if (cl->super_class == Nil){
		/* Root class */
		cl->isa->super_class = cl;
		cl->isa->isa = cl->isa;
	}
	
	if (unresolved_classes != Nil){
		unresolved_classes->unresolved_class_previous = cl;
	}
	cl->unresolved_class_next = unresolved_classes;
	unresolved_classes = cl;
}


PRIVATE void
objc_updateDtableForClassContainingMethod(Method m)
{
	Class nextClass;
	void *state = NULL;
	SEL sel = method_getName(m);
	for (;;) {
		nextClass = objc_class_next(objc_classes,
									(struct objc_class_table_enumerator**)&state);
		if (nextClass == Nil){
			break;
		}
		
		if (class_getInstanceMethodNonRecursive(nextClass, sel) == m){
			objc_update_dtable_for_class(nextClass);
			return;
		}
	}
}

Class
objc_allocateClassPairInModule(Class superclass, const char *name,
									 size_t extraBytes, void *module)
{
	if (name == NULL || *name == '\0'){
		objc_abort("Trying to create a class with NULL or empty name.");
	}
	
	if (superclass != Nil && !superclass->flags.resolved){
		if (!objc_class_resolve(superclass)){
			/*
			 * Cannot create a subclass of an unfinished class.
			 * The reason is simple: what if the superclass added
			 * a variable after the subclass did so?
			 */
			objc_abort("Trying to create a subclass of an unresolved "
					   "class.");
		}
		
		/* Got resolved. */
	}
	
	if (objc_class_table_get(objc_classes, name) != NULL){
		/* i.e. a class with this name already exists */
		objc_log("A class with this name already exists (%s).\n", name);
		return NULL;
	}
	
	Class newClass = (Class)objc_zero_alloc(sizeof(struct objc_class)
											+ extraBytes, M_CLASS_TYPE);
	Class newMetaClass = (Class)objc_zero_alloc(sizeof(struct objc_class),
												M_CLASS_TYPE);
	newClass->isa = newMetaClass;
	newClass->super_class = superclass;
	if (superclass == Nil){
		/* Creating a new root class */
		/* isa-loop for the root class */
		newMetaClass->isa = newMetaClass;
		/* It's a subclass of its own instance */
		newMetaClass->super_class = newClass;
	}else{
		newMetaClass->isa = superclass->isa;
		newMetaClass->super_class = superclass->isa;
	}
	
	newClass->name = newMetaClass->name = objc_strcpy(name);
	
	/*
	 * The instance size needs to be 0, as the root class
	 * is responsible for adding an isa ivar.
	 */
	newMetaClass->instance_size = sizeof(struct objc_class);
	newClass->instance_size = 0;
	if (superclass != Nil){
		newClass->instance_size = superclass->instance_size;
	}
	
	newMetaClass->flags.meta = YES;
	
	newMetaClass->flags.user_created = YES;
	newClass->flags.user_created = YES;
	
	newClass->dtable = newMetaClass->dtable = uninstalled_dtable;
	

	newClass->kernel_module = module;
	newMetaClass->kernel_module = module;
	
	
	/* It is inserted into the class tree and hash table on class_finish */
	
	return newClass;
}

Class
objc_allocateClassPair(Class superclass, const char *name, size_t extraBytes)
{
	/*
	 * We need to populate the module fields, however, requiring the user to
	 * specify the module is not very clean solution, so we instead use this
	 * hackery: get the caller function and determine which module it belongs
	 * to.
	 */
	void *caller_function = __builtin_return_address(1);
	void *module = objc_module_for_pointer(caller_function);
	return objc_allocateClassPairInModule(superclass, name, extraBytes, module);
}

void
objc_registerClassPair(Class cl)
{
	if (cl == Nil){
		objc_abort("Cannot finish a NULL class!\n");
		return;
	}
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	objc_debug_log("registering class pair %s\n", class_getName(cl));
	
	objc_class_insert(objc_classes, cl);
	objc_class_resolve(cl);
}

static inline void
_objc_deallocate_class_fields(Class cls)
{
	Class meta = cls->isa;
	free_dtable((dtable_t*)&cls->dtable);
	free_dtable((dtable_t*)&meta->dtable);
	
	if (cls->extra_space != NULL) {
		objc_class_extra_destroy_for_class(cls);
	}
	
	if (meta->extra_space != NULL) {
		objc_class_extra_destroy_for_class(meta);
	}
}

/*
 * Since loading categories copies the method list as well as adding methods
 * manually, we deallocate the method lists except for the last one, which is
 * the initial method list that was loaded with the class.
 */
static void
_objc_deallocate_method_list(objc_method_list *list)
{
	if (list == NULL){
		return;
	}
		
	_objc_deallocate_method_list(list->next);
	if (list->is_dynamically_allocated){
		objc_dealloc(list, M_METHOD_LIST_TYPE);
	}
}

void
objc_disposeClassPair(Class cls)
{
	if (cls == Nil){
		return;
	}
	
	Class meta = cls->isa;
	
	{
		OBJC_LOCK_RUNTIME_FOR_SCOPE();
		_objc_class_remove_from_class_tree(cls);
		_objc_class_remove_from_class_tree(meta);
		objc_class_remove(objc_classes, (void*)cls->name);
	}
	
	objc_method_list_free(cls->methods);
	objc_method_list_free(meta->methods);
	
	objc_ivar_list_free(cls->ivars);
	
	objc_protocol_list_free(cls->protocols);
	objc_protocol_list_free(meta->protocols);
	
	objc_property_list_free(cls->properties);
	
	_objc_deallocate_class_fields(cls);
	
	objc_dealloc(cls, M_CLASS_TYPE);
	objc_dealloc(meta, M_CLASS_TYPE);
}

Class *
objc_copyClassList(unsigned int *out_count)
{
	size_t class_count = objc_classes->table_used;
	
	Class *classes = objc_alloc(class_count * sizeof(Class), M_CLASS_TYPE);
	
	int count = 0;
	struct objc_class_table_enumerator *e = NULL;
	Class next;
	while (count < class_count &&
	       (next = objc_class_next(objc_classes, &e))){
		classes[count++] = next;
	}
	
	if (out_count != NULL){
		*out_count = count;
	}
	
	return classes;
}

int
objc_getClassList(Class *buffer, int len)
{
	int count = 0;
	struct objc_class_table_enumerator *e = NULL;
	Class next;
	while (count < len &&
	       (next = objc_class_next(objc_classes, &e))){
		buffer[count++] = next;
	}
	
	return count;
}

id
objc_getClass(const char *name)
{
	if (name == NULL){
		return nil;
	}
	
	Class c = objc_class_table_get(objc_classes, name);
	if (c == Nil){
		c = objc_class_lookup_hook(name);
	}
	
	return (id)c;
}

id
objc_lookup_class(const char *name)
{
	objc_debug_log("Class lookup: %s\n", name);
	return objc_getClass(name);
}

id
objc_lookUpClass(const char *name)
{
	if (name == NULL){
		return nil;
	}
	return (id)objc_class_table_get(objc_classes, name);
}

PRIVATE BOOL
objc_class_resolve(Class cl)
{
	if (cl->flags.resolved){
		return YES;
	}
	
	/* The superclass needs to be resolved. */
	Class superclass = cl->super_class;
	if (superclass != Nil && !superclass->flags.resolved){
		if (!objc_class_resolve(superclass)){
			return NO;
		}
	}
	
	/* Beyond this point, the superclasses are resolved. */
	
	_objc_class_remove_from_unresolved_list(cl);
	
	cl->flags.resolved = YES;
	cl->isa->flags.resolved = YES;
	
	_objc_insert_class_into_class_tree(cl);
	
	objc_class_send_load_messages(cl);
	
	objc_class_load_callback(cl);
	
	return YES;
}

PRIVATE void
objc_class_resolve_links(void)
{
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	objc_debug_log("resolving class links\n");
	
	Class cl = unresolved_classes;
	BOOL resolved_class;
	do {
		resolved_class = NO;
		while (cl != Nil) {
			/* Need to remember the next before resolving */
			Class next = cl->unresolved_class_next;
			
			objc_class_resolve(cl);
			resolved_class |= cl->flags.resolved;
			
			cl = next;
		}
		
		cl = unresolved_classes;
	} while (resolved_class);
}

PRIVATE void
objc_class_register_class(Class cl)
{
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	_objc_class_register_class_no_lock(cl);
}

void
objc_class_register_classes(Class *cl, unsigned int count)
{
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	for (int i = 0; i < count; ++i){
		_objc_class_register_class_no_lock(cl[i]);
	}
}

PRIVATE Class
objc_class_get_root_class_list(void)
{
	return class_tree;
}

#pragma mark -
#pragma mark Initializator-related

/*
 * Initializes the class extensions and internal class structures.
 */
void
objc_class_init(void)
{
	objc_debug_log("Initializing classes.\n");
	
	objc_classes =
	objc_class_table_create(OBJC_CLASS_TABLE_INITIAL_CAPACITY);
	objc_load_messages =
	objc_load_messages_table_create(OBJC_LOAD_TABLE_INITIAL_CAPACITY,
									"objc_load_messages");
}

static void
__objc_class_deallocate(Class cl)
{
	objc_debug_log("Deallocating class %s [%p].\n", cl->name, cl);
	if (cl->flags.user_created) {
		if (cl->flags.fake) {
			/* TODO: We shouldn't really be here at all! */
			SparseArrayDestroy((dtable_t*)&cl->dtable);
			objc_dealloc(cl, M_FAKE_CLASS_TYPE);
		}else{
			// TODO the rest of the stuff
			objc_disposeClassPair(cl);
		}
	}else{
		objc_class_remove(objc_classes, (void*)cl->name);
		
		_objc_deallocate_method_list(cl->methods);
		_objc_deallocate_class_fields(cl);
	}
}

PRIVATE void
objc_unload_class(Class cl)
{
	/* Assumes the runtime lock is held. */
	_objc_class_remove_from_class_tree(cl);
	_objc_class_remove_from_class_tree(cl->isa);
	
	/* It handles the meta class as well. */
	__objc_class_deallocate(cl);
}

void
objc_class_destroy(void)
{
	objc_debug_log("Destroying classes.\n");
	
	objc_class_table_destroy(objc_classes, __objc_class_deallocate);
	objc_load_messages_table_destroy(objc_load_messages, NULL);
}


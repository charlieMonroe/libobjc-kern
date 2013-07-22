#include "runtime.h"
#include "utils.h"
#include "selector.h"
#include "sarray2.h"
#include "dtable.h"
#include "class_registry.h"
#include "class.h"
#include "private.h"

/**
 * The initial capacities for the hash tables.
 */
#define OBJC_CLASS_TABLE_INITIAL_CAPACITY 512
#define OBJC_LOAD_TABLE_INITIAL_CAPACITY 512

#define MAP_TABLE_NAME objc_load_messages
#define MAP_TABLE_COMPARE_FUNCTION objc_pointers_are_equal
#define MAP_TABLE_HASH_KEY objc_hash_pointer
#define MAP_TABLE_HASH_VALUE objc_hash_pointer
#define MAP_TABLE_VALUE_TYPE IMP
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
		load_imp((id)cl, objc_load_selector);
		objc_load_messages_insert(objc_load_messages, load_imp);
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
	while (last_sibling->sibling_list != Nil){
		last_sibling = last_sibling->sibling_list;
	}
	
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
	/* Insert the class into the tree. */
	if (cl->super_class == Nil){
		/* Root class */
		if (class_tree == Nil){
			/* The first one, yay! */
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
		if (cl->subclass_list == Nil){
			// First subclass
			super_class->subclass_list = cl;
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
				return;
			}
			sibling = sibling->sibling_list;
		}
	}
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
		// Not on the list
		return;
	}
	
	if (cl->unresolved_class_previous == Nil){
		// The first class
		objc_assert(cl == unresolved_classes, "There are possibly two "
			    "unresolved class lists? (%p != %p)", cl,
			    unresolved_classes);
		unresolved_classes = cl->unresolved_class_next;
	}else{
		cl->unresolved_class_previous->unresolved_class_next =
						cl->unresolved_class_next;
	}
	
	if (cl->unresolved_class_next != Nil){
		// Not the end of the linked list
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
			size_t offset = size;
			if (size % ivar->align != 0){
				unsigned int padding = _padding_for_ivar(ivar,
									 offset);
				offset += padding;
			}
			ivar->offset = offset;
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
	
	objc_debug_log("Fixing up instance size of class %s%s - %d bytes\n",
		       cl->name, cl->flags.meta ? " (meta)" : "",
		       (unsigned int)cl->instance_size);
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
objc_allocateClassPair(Class superclass, const char *name, size_t extraBytes)
{
	if (name == NULL || *name == '\0'){
		objc_abort("Trying to create a class with NULL or empty name.");
	}
	
	if (superclass != Nil && !superclass->flags.resolved){
		/*
		 * Cannot create a subclass of an unfinished class.
		 * The reason is simple: what if the superclass added
		 * a variable after the subclass did so?
		 */
		objc_abort("Trying to create a subclass of an unresolved "
			   "class.");
	}
	
	if (objc_class_table_get(objc_classes, name) != NULL){
		/* i.e. a class with this name already exists */
		objc_log("A class with this name already exists (%s).\n", name);
		return NULL;
	}
	
	Class newClass = (Class)objc_zero_alloc(sizeof(struct objc_class)
						+ extraBytes);
	Class newMetaClass = (Class)objc_zero_alloc(sizeof(struct objc_class));
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
	
	/* It is inserted into the class tree and hash table on class_finish */
	
	return newClass;
}

void
objc_registerClassPair(Class cl)
{
	if (cl == Nil){
		objc_abort("Cannot finish a NULL class!\n");
		return;
	}
	
	OBJC_LOCK_RUNTIME();
		
	objc_class_insert(objc_classes, cl);
	objc_class_resolve(cl);
	
	OBJC_UNLOCK_RUNTIME();
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
	}
	
	objc_method_list_free(cls->methods);
	objc_method_list_free(meta->methods);
	
	objc_ivar_list_free(cls->ivars);
	
	objc_protocol_list_free(cls->protocols);
	objc_protocol_list_free(meta->protocols);
	
	objc_property_list_free(cls->properties);
	
	free_dtable(cls->dtable);
	free_dtable(meta->dtable);
	
	objc_class_table_set(objc_classes, cls->name, NULL);
	
	
	objc_dealloc(cls);
	objc_dealloc(meta);
}

Class *
objc_copyClassList(unsigned int *out_count)
{
	size_t class_count = objc_classes->table_used;
	
	Class *classes = objc_alloc(class_count * sizeof(Class));
	
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
	if (c != Nil){
		/* Found it */
		return (id)c;
	}
	
	// TODO aliases?
	// TODO hook
	
	return (id)c;
}

id
objc_lookUpClass(const char *name)
{
	if (name != NULL){
		return (id)objc_class_table_get(objc_classes, name);
	}
	return nil;
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
	
	// TODO resolving the superclass pointer char * -> Class - really necessary?
	
	cl->flags.resolved = YES;
	cl->isa->flags.resolved = YES;
	
	// TODO - fixup instance size here?
	_objc_class_fixup_instance_size(cl);
	_objc_class_fixup_instance_size(cl->isa);
	
	_objc_insert_class_into_class_tree(cl);
	
	objc_class_send_load_messages(cl);
	
	// TODO load call-back
	
	return YES;
}

PRIVATE void
objc_class_resolve_links(void)
{
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
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
	} while (resolved_class);
}

PRIVATE void
objc_class_register_class(Class cl)
{
	if (objc_class_table_get(objc_classes, cl->name) != Nil){
		objc_log("Class %s has been defined in multiple modules."
			 " Which one will be used is undefined.\n", cl->name);
		return;
	}
	
	objc_debug_log("Registering class %s with the runtime.\n", cl->name);
	
	objc_class_insert(objc_classes, cl);
	
	objc_register_selectors_from_class(cl, cl->isa);
	
	cl->dtable = cl->isa->dtable = uninstalled_dtable;
	
	if (cl->super_class == Nil){
		/* Root class */
		cl->isa->super_class = cl;
		cl->isa->isa = cl->isa;
	}
}

void
objc_class_register_classes(Class *cl, unsigned int count)
{
	for (int i = 0; i < count; ++i){
		objc_class_register_class(cl[i]);
	}
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
		objc_load_messages_table_create(OBJC_LOAD_TABLE_INITIAL_CAPACITY);
}

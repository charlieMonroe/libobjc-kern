#include "runtime.h"
#include "utils.h"
#include "selector.h"
#include "sarray2.h"
#include "dtable.h"
#include "class_registry.h"

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

/**
 * A hash table with the load messages.
 */
static objc_load_messages_table *objc_load_messages;

PRIVATE void objc_class_send_load_messages(Class cl){
	Class meta = cl->isa; // It's a class method, need to link at the meta class
	objc_method_list *list = meta->methods;
	while (list != NULL) {
		for (int i = 0; i < list->size; ++i){
			Method m = &list->method_list[i];
			if (m->selector == objc_load_selector){
				IMP load_imp = m->implementation;
				if (objc_load_messages_table_get(objc_load_messages, load_imp)){
					load_imp((id)cl, objc_load_selector);
					objc_load_messages_insert(objc_load_messages, load_imp);
				}
			}
		}
		list = list->next;
	}
}


// Forward declarations needed for the hash table
static inline BOOL _objc_class_name_is_equal_to(void *key, Class cl);
static inline uint32_t _objc_class_hash(Class cl);

#define MAP_TABLE_NAME objc_class
#define MAP_TABLE_COMPARE_FUNCTION _objc_class_name_is_equal_to
#define MAP_TABLE_HASH_KEY objc_hash_string
#define MAP_TABLE_HASH_VALUE _objc_class_hash
#define MAP_TABLE_VALUE_TYPE Class
#include "hashtable.h"

/**
 * A hash map with the classes.
 */
objc_class_table *objc_classes;

/**
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

/**
 * A pointer to the first unresolved class. The rest of
 * the unresolved classes are a linked list using
 * the unresolved pointer in the Class structure.
 */
static Class unresolved_classes;

/**
 * The runtime lock used for locking when modifying the
 * class hierarchy or modifying the classes in general.
 */
objc_rw_lock objc_runtime_lock;


OBJC_INLINE BOOL _objc_class_name_is_equal_to(void *key, Class cl){
	return objc_strings_equal(cl->name, (const char*)key);
}

OBJC_INLINE uint32_t _objc_class_hash(Class cl){
	// Hash the name
	return objc_hash_string(cl->name);
}

/**
 * Inserts cl into the siblings linked list
 */
static inline void _objc_insert_class_to_back_of_sibling_list(Class cl, Class sibling){
	// Inserting into the linked list
	Class last_sibling = sibling->sibling_list;
	while (last_sibling->sibling_list != Nil){
		last_sibling = last_sibling->sibling_list;
	}
	
	// Add it to the end of the list
	last_sibling->sibling_list = cl;
	// And for the meta classes as well
	last_sibling->isa->sibling_list = cl->isa;
}

/**
 * Inserts a class into the class tree. Note that the run-time
 * lock must be held.
 */
static inline void _objc_insert_class_into_class_tree(Class cl){
	// Insert the class into the tree.
	if (cl->super_class == Nil){
		// Root class
		if (class_tree == Nil){
			// The first one, yay!
			class_tree = cl;
		}else{
			_objc_insert_class_to_back_of_sibling_list(cl, class_tree);
		}
	}else{
		/**
		 * It is a subclass, so need to insert it into the
		 * subclass list of the superclass as well as into
		 * the siblings list of the subclasses.
		 */
		Class super_class = cl->super_class;
		if (cl->subclass_list == Nil){
			// First subclass
			super_class->subclass_list = cl;
		}else{
			_objc_insert_class_to_back_of_sibling_list(cl, super_class->subclass_list);
		}
	}
}

/**
 * Removes the class from the unresolved list.
 */
static inline void _objc_class_remove_from_unresolved_list(Class cl){
	if (cl->unresolved_class_next == Nil &&
	    cl->unresolved_class_previous == Nil &&
	    cl != unresolved_classes){
		// Not on the list
		return;
	}
	
	if (cl->unresolved_class_previous == Nil){
		// The first class
		objc_assert(cl == unresolved_classes, "There are possibly two unresolved class lists? (%p != %p)", cl, unresolved_classes);
		unresolved_classes = cl->unresolved_class_next;
	}else{
		cl->unresolved_class_previous->unresolved_class_next = cl->unresolved_class_next;
	}
	
	if (cl->unresolved_class_next != Nil){
		// Not the end of the linked list
		cl->unresolved_class_next->unresolved_class_previous = cl->unresolved_class_previous;
	}
	
	cl->unresolved_class_previous = Nil;
	cl->unresolved_class_next = Nil;
}

// Forward declaration
static void _objc_class_fixup_instance_size(Class cl);

static unsigned int _objc_class_calculate_instance_size(Class cl){
	unsigned int size = 0;
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
			Ivar ivar = &cl->ivars->ivar_list[i];
			unsigned int offset = size;
			if (size % ivar->align != 0){
				unsigned int padding = (ivar->align - (size % ivar->align));
				offset += padding;
				size += padding;
			}
			ivar->offset = offset;
			size += ivar->size;
			
			/**
			 * The ->ivar_offsets can be generated by the compiler
			 * since the number of ivars in this class is known at compile
			 * time.
			 */
			cl->ivar_offsets[i] = offset;
			
		}
	}
	return size;
}

static void _objc_class_fixup_instance_size(Class cl){
	objc_assert(cl != Nil, "Cannot fixup instance size of Nil class!\n");
	
	if (cl->flags.meta){
		// Meta class has the size of the class structure
		cl->instance_size = sizeof(struct objc_class);
	}else{
		cl->instance_size = _objc_class_calculate_instance_size(cl);
	}
	
	objc_debug_log("Fixing up instance size of class %s%s - %d bytes\n", cl->name, cl->flags.meta ? " (meta)" : "", cl->instance_size);
}


Class objc_class_create(Class superclass, const char *name) {
	if (name == NULL || *name == '\0'){
		objc_abort("Trying to create a class with NULL or empty name.");
	}
	
	if (superclass != Nil && !superclass->flags.resolved){
		/** Cannot create a subclass of an unfinished class.
		 * The reason is simple: what if the superclass added
		 * a variable after the subclass did so?
		 */
		objc_abort("Trying to create a subclass of an unresolved class.");
	}
	
	OBJC_LOCK_RUNTIME();
	if (objc_class_table_get(objc_classes, name) != NULL){
		/* i.e. a class with this name already exists */
		objc_log("A class with this name already exists (%s).\n", name);
		OBJC_UNLOCK_RUNTIME();
		return NULL;
	}
	
	Class newClass = (Class)(objc_zero_alloc(sizeof(struct objc_class)));
	Class newMetaClass = (Class)(objc_zero_alloc(sizeof(struct objc_class)));
	newClass->isa = newMetaClass;
	newClass->super_class = superclass;
	if (superclass == Nil){
		// Creating a new root class
		newMetaClass->isa = newMetaClass; // isa-loop for the root class
		newMetaClass->super_class = newClass; // It's a subclass of its own instance
	}else{
		// Set the isa to the superclass's meta class name,
		// it will be resolved later
		newMetaClass->isa = (Class)superclass->isa->name;
		newMetaClass->super_class = superclass->isa;
	}
	
	// TODO - copy the name twice?
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
	
	// It is inserted into the class tree and hash table on class_finish
	
	OBJC_UNLOCK_RUNTIME();
	
	return newClass;
}

void objc_class_finish(Class cl){
	if (cl == Nil){
		objc_abort("Cannot finish a NULL class!\n");
		return;
	}
	
	OBJC_LOCK_RUNTIME();
		
	objc_class_insert(objc_classes, cl);
	objc_class_resolve(cl);
	
	OBJC_UNLOCK_RUNTIME();
}

Class *objc_class_copy_list(unsigned int *out_count){
	size_t class_count = objc_classes->table_used;
	
	// NULL-terminated
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
int objc_class_get_list(Class *buffer, unsigned int len){
	int count = 0;
	struct objc_class_table_enumerator *e = NULL;
	Class next;
	while (count < len &&
	       (next = objc_class_next(objc_classes, &e))){
		buffer[count++] = next;
	}
	
	return count;
}

Class objc_class_for_name(const char *name){
	if (name == NULL){
		return Nil;
	}
	
	Class c = objc_class_table_get(objc_classes, name);
	if (c != Nil){
		/* Found it */
		return c;
	}
	
	// TODO aliases?
	// TODO hook
	
	return c;
}

Class objc_class_look_up(const char *name){
	if (name != NULL){
		return objc_class_table_get(objc_classes, name);
	}
	return Nil;
}

PRIVATE BOOL objc_class_resolve(Class cl){
	if (cl->flags.resolved){
		return YES;
	}
	
	/**
	 * The class needs to be resolved.
	 */
	Class superclass = cl->super_class;
	if (superclass != Nil && !superclass->flags.resolved){
		if (!objc_class_resolve(superclass)){
			return NO;
		}
	}
	
	/**
	 * Beyond this point, the superclasses are resolved.
	 */
	
	_objc_class_remove_from_unresolved_list(cl);
	
	// TODO resolving the superclass pointer char * -> Class - really necessary?
	
	cl->flags.resolved = YES;
	cl->isa->flags.resolved = YES;
	
	// TODO - here?
	_objc_class_fixup_instance_size(cl);
	_objc_class_fixup_instance_size(cl->isa);
	
	_objc_insert_class_into_class_tree(cl);
	
	objc_class_send_load_messages(cl);
	
	// TODO load call-back
	
	return YES;
}

PRIVATE void objc_class_resolve_links(void){
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	Class cl = unresolved_classes;
	BOOL resolved_class;
	do {
		resolved_class = NO;
		while (cl != Nil) {
			// Need to remember the next before resolving
			Class next = cl->unresolved_class_next;
			
			objc_class_resolve(cl);
			resolved_class |= cl->flags.resolved;
			
			cl = next;
		}
	} while (resolved_class);
}

PRIVATE void objc_class_register_class(Class cl){
	if (objc_class_table_get(objc_classes, cl->name) != Nil){
		objc_log("Class %s has been defined in multiple modules."
			 " Which one will be used is undefined.\n", cl->name);
		return;
	}
	
	objc_debug_log("Registering class %s with the runtime.\n", cl->name);
	
	objc_class_insert(objc_classes, cl);
	
	objc_register_selectors_from_class(cl);
	objc_register_selectors_from_class(cl->isa);
	
	cl->dtable = cl->isa->dtable = uninstalled_dtable;
	
	if (cl->super_class == Nil){
		// Root class
		cl->isa->super_class = cl;
		cl->isa->isa = cl->isa;
	}
}

void objc_class_register_classes(Class *cl, unsigned int count){
	for (int i = 0; i < count; ++i){
		objc_class_register_class(cl[i]);
	}
}

/***** INITIALIZATION *****/
#pragma mark -
#pragma mark Initializator-related

/**
 * Initializes the class extensions and internal class structures.
 */
void objc_class_init(void){
	objc_debug_log("Initializing classes.\n");
	
	objc_classes = objc_class_table_create(OBJC_CLASS_TABLE_INITIAL_CAPACITY);
	objc_load_messages = objc_load_messages_table_create(OBJC_LOAD_TABLE_INITIAL_CAPACITY);
}

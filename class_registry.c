#include "os.h"
#include "types.h"
#include "runtime.h"
#include "utils.h"
#include "selector.h"
#include "sarray2.h"
#include "dtable.h"

/**
 * The initial capacity for the hash table.
 */
#define OBJC_CLASS_TABLE_INITIAL_CAPACITY 256

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


Class objc_class_create(Class superclass, const char *name) {
	if (name == NULL || *name == '\0'){
		objc_abort("Trying to create a class with NULL or empty name.");
	}
	
	if (superclass != Nil && superclass->flags.in_construction){
		/** Cannot create a subclass of an unfinished class.
		 * The reason is simple: what if the superclass added
		 * a variable after the subclass did so?
		 */
		objc_abort("Trying to create a subclass of an unfinished class.");
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
	
	newMetaClass->flags.in_construction = YES;
	newMetaClass->flags.meta = YES;
	newClass->flags.in_construction = YES;
	
	newMetaClass->flags.user_created = YES;
	newClass->flags.user_created = YES;
	
	// TODO newClass and newMetaClass dtable
	
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
	
	/* That's it! Just mark it as not in construction */
	cl->flags.in_construction = NO;
	
	// Fix up the class name
	Class metaClass = cl->isa;
	Class metaMeta = objc_class_table_get(objc_classes, metaClass->isa);
	metaClass->isa = metaMeta;
	metaClass->flags.in_construction = NO;
	
	objc_class_insert(objc_classes, cl);
	_objc_insert_class_into_class_tree(cl);
	
	OBJC_UNLOCK_RUNTIME();
}

Class *objc_class_get_list(void){
	size_t class_count = objc_classes->table_used;
	
	// NULL-terminated
	Class *classes = objc_alloc((class_count + 1) * sizeof(Class));
	
	int count = 0;
	struct objc_class_table_enumerator *e = NULL;
	Class next;
	while (count < class_count &&
	       (next = objc_class_next(objc_classes, &e))){
		classes[count++] = next;
	}
	
	// NULL-termination
	classes[count] = NULL;
	return classes;
}

Class objc_class_for_name(const char *name){
	if (name == NULL){
		return Nil;
	}
	
	Class c = objc_class_table_get(objc_classes, name);
	if (c == NULL || c->flags.in_construction){
		/* NULL, or still in construction */
		return Nil;
	}
	
	return c;
}

void objc_class_register_class(Class cl){
	if (objc_class_table_get(objc_classes, cl->name) != Nil){
		objc_log("Class %s has been defined in multiple modules."
			 " Which one will be used is undefined.\n", cl->name);
		return;
	}
	
	objc_debug_log("Registering class %s with the runtime.\n", cl->name);
	
	objc_class_insert(objc_classes, cl);
	
	objc_register_selectors_from_class(cl);
	objc_register_selectors_from_class(cl->isa);
	
	// TODO dtable
	
	if (cl->super_class == Nil){
		// Root class
		cl->isa->super_class = cl;
		cl->isa->isa = cl->isa;
	}
	
	cl->dtable = uninstalled_dtable;
	add_method_list_to_class(cl, cl->methods);
	cl->isa->dtable = uninstalled_dtable;
	add_method_list_to_class(cl->isa, cl->isa->methods);
	
	cl->flags.in_construction = NO;
	cl->isa->flags.in_construction = NO;
	
	
	// TODO other stuff
	
}

void objc_class_register_classes(Class *cl, unsigned int count){
	for (int i = 0; i < count; ++i){
		objc_class_register_class(cl[i]);
	}
}

BOOL objc_resolve_class(Class cl){
	// TODO
	return NO;
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
}

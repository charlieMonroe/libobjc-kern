/**
 * Implementation of the class-related functions.
 */

#include "private.h"
#include "os.h"
#include "utils.h"
#include "selector.h"
#include "method.h"
#include "sarray2.h"

#pragma mark -
#pragma mark STATIC_VARIABLES_AND_MACROS

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
 * Note, however, that each sibling level is actually
 * a circular linked list so that inserting siblings is
 * fairly easy.
 *
 * The class doesn't get added to the tree until it is
 * finished (or loaded via loader).
 */
static Class class_tree;

/**
 * A lock that is used for manipulating with classes - e.g. adding a class
 * to the run-time, etc.
 *
 * All actions performed with this lock locked, should be rarely performed,
 * or at least performed seldomly.
 */
objc_rw_lock objc_runtime_lock;

/**
 * A cached forwarding selectors.
 */
static SEL objc_forwarding_selector;
static SEL objc_drops_unrecognized_forwarding_selector;

/**
 * A function that is returned when the receiver is nil.
 */
static id _objc_nil_receiver_function(id self, SEL _cmd, ...){
	return nil;
}

/**
 * Class structures are versioned. If a class prototype of a different
 * version is encountered, it is either ignored, if the version is
 * 
 */
#define OBJC_MAX_CLASS_VERSION_SUPPORTED ((unsigned int)0)


#pragma mark -
#pragma mark Private Functions

OBJC_INLINE BOOL _objc_class_name_is_equal_to(void *key, Class cl){
	return objc_strings_equal(cl->name, (const char*)key);
}

OBJC_INLINE uint32_t _objc_class_hash(Class cl){
	// Hash the name
	return objc_hash_string(cl->name);
}


/**
 * Returns the size required for instances of class 'cl'. This
 * includes the extra space required by extensions.
 */
OBJC_INLINE unsigned int _instance_size(Class cl){
	if (cl->flags.is_meta){
		cl = objc_class_for_name(cl->name);
	}
	return cl->instance_size;
}

/**
 * Searches for a method in a method list and returns it, or NULL 
 * if it hasn't been found.
 */
OBJC_INLINE Method _lookup_method_in_method_list(objc_method_list *method_list, SEL selector){
	if (method_list == NULL){
		return NULL;
	}
	
	while (method_list != NULL) {
		int i;
		for (i = 0; i < method_list->size; ++i){
			Method m = &method_list->method_list[i];
			if (m->selector == selector){
				return m;
			}
		}
		method_list = method_list->next;
	}
	
	// Nothing found.
	return NULL;
}

/**
 * Searches for an instance method for a selector.
 */
OBJC_INLINE Method _lookup_method(Class class, SEL selector){
	Method m;
	
	if (class == Nil || selector == 0){
		return NULL;
	}
	
	while (class != NULL){
		m = _lookup_method_in_method_list(class->methods, selector);
		if (m != NULL){
			return m;
		}
		class = class->super_class;
	}
	return NULL;
}

OBJC_INLINE Method _lookup_cached_method(Class cl, SEL selector){
	if (cl != Nil && selector != 0 && cl->cache != NULL){
		return (Method)SparseArrayLookup((SparseArray*)cl->cache, selector);
	}
	return NULL;
}

OBJC_INLINE void _cache_method(Class cl, Method m){
	// TODO locking on creation
	// TODO not method, use a slot
	if (cl != Nil && m != NULL && cl->cache != NULL){
		SparseArrayInsert((SparseArray*)cl->cache, m->selector, m);
	}
}

/**
 * Looks up an instance method implementation within a class.
 * NULL if it hasn't been found, yet no-op function in case
 * the receiver is nil.
 */
OBJC_INLINE IMP _lookup_method_impl(Class cl, SEL selector){
	Method m = _lookup_cached_method(cl, selector);
	if (m != NULL){
		return m->implementation;
	}
	
	m = _lookup_method(cl, selector);
	if (m == NULL){
		return NULL;
	}
	
	_cache_method(cl, m);
	return m->implementation;
}

/**
 * Returns whether cl is a subclass of superclass_candidate.
 */
OBJC_INLINE BOOL _class_is_subclass_of_class(Class cl, Class superclass_candidate){
	while (cl != Nil) {
		if (cl->super_class == superclass_candidate){
			return YES;
		}
		cl = cl->super_class;
	}
	return NO;
}


/**
 * Adds methods from the array 'm' into the method_list. The m array doesn't
 * have to be NULL-terminated, and has to contain 'count' methods.
 *
 * The m array is copied over to be NULL-terminated, but the Method 'objects'
 * are not.
 */
OBJC_INLINE void _add_methods_to_class(Class cl, Method *m, unsigned int count){
	
	// TODO locking
	objc_method_list *list = objc_method_list_create(count);
	for (int i = 0; i < count; ++i){
		list->method_list[i] = *m[i];
	}
	
	cl->methods = objc_method_list_prepend(cl->methods, list);
}


/**
 * Adds instance methods to class cl.
 */
OBJC_INLINE void _add_methods(Class cl, Method *m, unsigned int count){
	if (cl == NULL || m == NULL){
		return;
	}
	
	_add_methods_to_class(cl, m, count);
	
	/**
	 * Unfortunately, as the cl's superclass might
	 * have implemented this method, which might
	 * have been cached by its subclasses.
	 *
	 * First, we figure out, it it really is this scenario.
	 *
	 * If it is, we need to find all subclasses and flush
	 * their caches.
	 */
	// TODO
}

/**
 * Crashes the program because forwarding isn't supported by the class of the object.
 */
OBJC_INLINE void _forwarding_not_supported_abort(id obj, SEL selector){
	/* i.e. the object doesn't respond to the
	 forwarding selector either. */
	objc_log("%s doesn't support forwarding and doesn't respond to selector %s!\n", obj->isa->name, objc_selector_get_name(selector));
	objc_abort("Class doesn't support forwarding.");
}

/**
 * The first part of the forwarding mechanism. obj is called a method
 * forwardedMethodForSelector: which should return a Method pointer
 * to the actual method to be called. If it doesn't wish to forward this 
 * particular selector, return NULL. Note that the Method doesn't need
 * to be registered anywhere, it may be a faked pointer.
 *
 * If NULL is returned, the run-time moves on to the second step:
 * asking the object whether to drop the message (a nil-receiver
 * method is returned), or whether to raise an exception.
 */
OBJC_INLINE Method _forward_method_invocation(id obj, SEL selector){
	if (objc_selectors_equal(selector, objc_forwarding_selector) || objc_selectors_equal(selector, objc_drops_unrecognized_forwarding_selector)){
		/* Make sure the app really crashes and doesn't create an infinite cycle. */
		return NULL;
	}else{
		/* Forwarding. */
		IMP forwarding_imp;
		
		if (objc_forwarding_selector == 0){
			// TODO
		}
		
		forwarding_imp = _lookup_method_impl(obj->isa, objc_forwarding_selector);
		if (forwarding_imp == NULL){
			objc_log("Class %s doesn't respond to selector %s.\n", obj->isa->name, objc_selector_get_name(selector));
			return NULL;
		}
		
		return ((Method(*)(id,SEL,SEL))forwarding_imp)(obj, objc_forwarding_selector, selector);
	}
}

/**
 * Looks for an ivar named name in ivar_list.
 */
OBJC_INLINE Ivar _ivar_named_in_ivar_list(objc_ivar_list *ivar_list, const char *name){
	if (ivar_list == NULL){
		return NULL;
	}
	
	for (int i = 0; i < ivar_list->size; ++i){
		Ivar var = &ivar_list->ivar_list[i];
		if (objc_strings_equal(var->name, name)){
			return var;
		}
	}
	
	return NULL;
}

/**
 * Finds an Ivar in class with name.
 */
OBJC_INLINE Ivar _ivar_named(Class cl, const char *name){
	if (name == NULL){
		return NULL;
	}
	
	while (cl != Nil){
		Ivar var = _ivar_named_in_ivar_list(cl->ivars, name);
		if (var != NULL){
			return var;
		}
		cl = cl->super_class;
	}
	return NULL;
}

/**
 * Returns number of ivars on class cl. Used only
 * by the functions copying ivar lists.
 */
OBJC_INLINE unsigned int _ivar_count(Class cl){
	objc_ivar_list *ivar_list = cl->ivars;
	if (ivar_list == NULL){
		return 0;
	}else{
		return ivar_list->size;
	}
}

/**
 * Copies over ivars declared on cl into list.
 */
OBJC_INLINE void _ivars_copy_to_list(Class cl, Ivar *list, unsigned int max_count){
	unsigned int counter = 0;
	objc_ivar_list *ivar_list = cl->ivars;
	
	if (ivar_list == NULL){
		/* NULL-termination */
		list[0] = NULL;
		return;
	}
	
	for (int i = 0; i < ivar_list->size; ++i){
		if (counter >= max_count) {
			break;
		}
		
		list[counter] = &ivar_list->ivar_list[i];
		++counter;
	}
	
	/* NULL termination */
	list[max_count] = NULL;
}

/**
 * Looks up method. If obj is nil, returns the nil receiver method.
 *
 * If the method is not found, forwarding takes place.
 */
OBJC_INLINE Method _lookup_object_method(id obj, SEL selector){
	Method method = NULL;
	
	if (obj == nil){
		// TODO
		return NULL;
	}
	
	method = _lookup_cached_method(obj->isa, selector);
	if (method != NULL){
		return method;
	}
	
	method = _lookup_method(obj->isa, selector);
	if (method != NULL){
		_cache_method(obj->isa, method);
	}
	
	if (method == NULL){
		/* Not found! Prepare for forwarding. */
		Method forwarded_method = _forward_method_invocation(obj, selector);
		if (forwarded_method != NULL){
			/** The object returned a method 
			 * that should be called instead.
			 */
			
			/** ++ to invalidate inline cache. */
			++forwarded_method->version;
			return forwarded_method;
		}
		
		if (forwarded_method == NULL){
			// TODO
			return NULL;
		}
		
		_forwarding_not_supported_abort(obj, selector);
		return NULL;
	}
	
	return method;
}

/**
 * The same scenario as above, but in this case a call to the superclass.
 */
OBJC_INLINE Method _lookup_method_super(objc_super *sup, SEL selector){
	Method method;
	
	if (sup == NULL){
		return NULL;
	}
	
	if (sup->receiver == nil){
		// TODO
		return NULL;
	}
	
	method = _lookup_method(sup->class, selector);
	
	if (method == NULL){
		/* Not found! Prepare for forwarding. */
		objc_log("Called super to class %s, which doesn't implement selector %s.\n", sup->class->name, objc_selector_get_name(selector));
		if (_forward_method_invocation(sup->receiver, selector)){
			// TODO
			return NULL;
		}else{
			_forwarding_not_supported_abort(sup->receiver, selector);
			return NULL;
		}
	}
	
	return method;
}

/**
 * Validates the class prototype.
 */
/*
OBJC_INLINE BOOL _validate_prototype(struct objc_class_prototype *prototype){
	if (prototype->name == NULL || objc_strlen(prototype->name) == 0){
		objc_log("Trying to register a prototype of a class with NULL or empty name.\n");
		return NO;
	}
	
	if (objc_class_holder_lookup(objc_classes, prototype->name) != NULL){
		objc_log("Trying to register a prototype of class %s, but such a class has already been registered.\n", prototype->name);
		return NO;
	}
	
	if (prototype->super_class_name != NULL && objc_class_holder_lookup(objc_classes, prototype->super_class_name) == NULL){
		objc_log("Trying to register a prototype of class %s, but its superclass %s hasn't been registered yet.\n", prototype->name, prototype->super_class_name);
		return NO;
	}
	
	if (prototype->isa != NULL){
		objc_log("Trying to register a prototype of class %s that has non-NULL isa pointer.\n", prototype->name);
		return NO;
	}
	
	if (prototype->instance_cache != NULL || prototype->class_cache != NULL){
		objc_log("Trying to register a prototype of class %s that already has a non-NULL cache.\n", prototype->name);
		return NO;
	}
	
	if (prototype->version > OBJC_MAX_CLASS_VERSION_SUPPORTED){
		objc_log("Trying to register a prototype of class %s of a future version (%u).\n", prototype->name, prototype->version);
		return NO;
	}
	
	return YES;
}
 */

/**
 * Adds an ivar to class from ivar prototype list.
 */
/*
 OBJC_INLINE void _add_ivars_from_prototype(Class cl, Ivar *ivars){
	if (cl->super_class != Nil){
		cl->instance_size = cl->super_class->instance_size;
	}
	
	if (ivars == NULL){
		// No ivars.
		return;
	}
	
	cl->ivars = objc_array_create();
	
	while (*ivars != NULL) {
		objc_array_append(cl->ivars, *ivars);
		cl->instance_size += (*ivars)->size;
		
		++ivars;
	}
}
*/

/**
 * Registers a prototype and returns the resulting Class.
 */
/*
 OBJC_INLINE Class _register_prototype(struct objc_class_prototype *prototype){
	Class cl;
	unsigned int extra_space;
	
	// First, validation.
	if (!_validate_prototype(prototype)){
		return Nil;
	}
	
	if (prototype->version != OBJC_MAX_CLASS_VERSION_SUPPORTED){
		**
		 * This is the place where in the future, if the class
		 * prototype gets modified, any compatibility transformations
		 * should be performed.
		 *
		objc_log("This run-time doesn't have implemented class prototype conversion %u -> %u.\n", prototype->version, OBJC_MAX_CLASS_VERSION_SUPPORTED);
		objc_abort("Cannot convert class prototype.\n");
	}
	
	cl = (Class)prototype;
	
	**
	 * What needs to be done:
	 *
	 * 1) Lookup and connect superclass.
	 * 2) Connect isa.
	 * 3) Transform class method prototypes to objc_array.
	 * 4) Ditto with instance methods.
	 * 5) Add ivars and calculate instance size.
	 * 6) Allocate extra space and register with extensions.
	 * 7) Mark as not in construction.
	 * 8) Add to the class lists.
	 *
	
	if (prototype->super_class_name != NULL){
		cl->super_class = objc_class_holder_lookup(objc_classes, prototype->super_class_name);
	}
	
	cl->isa = cl;
	
	cl->class_methods = objc_method_transform_method_prototypes(prototype->class_methods);
	cl->instance_methods = objc_method_transform_method_prototypes(prototype->instance_methods);
	
	_add_ivars_from_prototype(cl, prototype->ivars);
	
	extra_space = _extra_class_space_for_extensions();
	if (extra_space != 0){
		cl->extra_space = objc_zero_alloc(extra_space);
	}else{
		cl->extra_space = NULL;
	}
	
	_register_class_with_extensions(cl);
	
	cl->flags.in_construction = NO;
	
	objc_class_holder_insert(objc_classes, cl);
	objc_array_append(objc_classes_array, cl);
	
	return cl;
}
*/

/***** PUBLIC FUNCTIONS *****/
/* Documentation in the header file. */

#pragma mark -
#pragma mark Adding methods


void objc_class_add_method(Class cl, Method m){
	if (cl == NULL || m == NULL){
		return;
	}
	
	_add_methods(cl, &m, 1);
}
void objc_class_add_methods(Class cl, Method *m, unsigned int count){
	_add_methods(cl, m, count);
}

#pragma mark -
#pragma mark Replacing methods

IMP objc_class_replace_method_implementation(Class cls, SEL name, IMP imp, const char *types){
	Method m;
	
	if (cls == Nil || name == 0 || imp == NULL || types == NULL){
		return NULL;
	}
	
	m = _lookup_method_in_method_list(cls->methods, name);
	if (m == NULL){
		Method new_method = objc_method_create(name, imp);
		_add_methods(cls, &new_method, 1);
		
		/**
		 * Method flushing is handled by the function adding methods.
		 */
	}else{
		m->implementation = imp;
		++m->version;
		
		/**
		 * There's no need to flush any caches as the whole
		 * Method pointer is cached -> hence the IMP
		 * pointer changes even inside the cache.
		 */
	}
	return m == NULL ? NULL : m->implementation;
}

#pragma mark -
#pragma mark Creating classes

Class objc_class_create(Class superclass, const char *name) {
	Class newClass;
	
	/** Check if the run-time has been initialized. */
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
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
	
	objc_rw_lock_wlock(&objc_runtime_lock);
	if (objc_class_table_get(objc_classes, name) != NULL){
		/* i.e. a class with this name already exists */
		objc_log("A class with this name already exists (%s).\n", name);
		objc_rw_lock_unlock(&objc_runtime_lock);
		return NULL;
	}
	
	newClass = (Class)(objc_zero_alloc(sizeof(struct objc_class)));
	newClass->isa = newClass; /* A loop to self to detect class method calls. */
	newClass->super_class = superclass;
	newClass->name = objc_strcpy(name);
	
	/*
	 * The instance size needs to be 0, as the root class
	 * is responsible for adding an isa ivar.
	 */
	newClass->instance_size = 0;
	if (superclass != Nil){
		newClass->instance_size = superclass->instance_size;
	}
	
	newClass->flags.in_construction = YES;
	
	objc_class_table_set(objc_classes, name, newClass);
	// TODO - add the class to some kind of array
	
	objc_rw_lock_unlock(&objc_runtime_lock);
	
	return newClass;
}
void objc_class_finish(Class cl){
	if (cl == Nil){
		objc_abort("Cannot finish a NULL class!\n");
		return;
	}
	
	// Insert the class into the tree.
	if (cl->super_class == Nil){
		// Root class
		if (class_tree == Nil){
			// The first one, yay!
			class_tree = cl;
			cl->sibling_list = class_tree; // A circular reference
		}else{
			// Inserting into the circular linked list
			cl->sibling_list = class_tree->sibling_list;
			class_tree->sibling_list = cl;
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
			cl->sibling_list = cl; // Circular
		}else{
			// There already are subclasses, join the circle
			Class sibling = super_class->subclass_list;
			cl->sibling_list = sibling->sibling_list;
			sibling->sibling_list = cl;
		}
	}
	
	/* That's it! Just mark it as not in construction */
	cl->flags.in_construction = NO;
}

#pragma mark -
#pragma mark Responding to selectors

BOOL objc_class_responds_to_selector(Class cl, SEL selector){
	return _lookup_method(cl, selector) != NULL;
}


#pragma mark -
#pragma mark Regular lookup functions

Method objc_lookup_method(Class cl, SEL selector){
	return _lookup_method(cl, selector);
}
IMP objc_lookup_method_impl(Class cl, SEL selector){
	/** No forwarding here! This is simply to lookup 
	 * a method implementation.
	 */
	return _lookup_method_impl(cl, selector);
}

#pragma mark -
#pragma mark Setting class of an object

Class objc_object_set_class(id obj, Class new_class){
	Class old_class;
	if (obj == nil){
		return Nil;
	}
	old_class = obj->isa;
	obj->isa = new_class;
	return old_class;
}

#pragma mark -
#pragma mark Object creation, copying and destruction

id objc_class_create_instance(Class cl){
	id obj;
	unsigned int size;
	
	if (cl->flags.in_construction){
		objc_log("Trying to create an instance of unfinished class (%s).", cl->name);
		return nil;
	}
	
	size = _instance_size(cl);
	
	obj = (id)objc_zero_alloc(size);
	obj->isa = cl;
	
	return obj;
}
void objc_object_deallocate(id obj){
	unsigned int size_of_obj;
	
	if (obj == nil){
		return;
	}
	
	size_of_obj = _instance_size(obj->isa);
	objc_dealloc(obj);
}

id objc_object_copy(id obj){
	id copy;
	unsigned int size;
	
	if (obj == nil){
		return nil;
	}
	
	size = _instance_size(obj->isa);
	copy = objc_zero_alloc(size);
	
	objc_copy_memory(obj, copy, size);
	
	return copy;
}

#pragma mark -
#pragma mark Object lookup

Method objc_object_lookup_method(id obj, SEL selector){
	return _lookup_object_method(obj, selector);
}
Method objc_object_lookup_method_super(objc_super *sup, SEL selector){
	return _lookup_method_super(sup, selector);
}
IMP objc_object_lookup_impl(id obj, SEL selector){
	return _lookup_object_method(obj, selector)->implementation;
}
IMP objc_object_lookup_impl_super(objc_super *sup, SEL selector){
	return _lookup_method_super(sup, selector)->implementation;
}

/***** INFORMATION GETTERS *****/
#pragma mark -
#pragma mark Information getters

BOOL objc_class_in_construction(Class cl){
	return cl->flags.in_construction;
}
const char *objc_class_get_name(Class cl){
	return cl->name;
}
Class objc_class_get_superclass(Class cl){
	return cl->super_class;
}
Class objc_object_get_class(id obj){
	return obj == nil ? Nil : obj->isa;
}
unsigned int objc_class_instance_size(Class cl){
	return _instance_size(cl);
}
Method *objc_class_get_class_method_list(Class cl){
	if (cl == Nil){
		return NULL;
	}
	return objc_method_list_flatten(cl->methods);
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
	Class c;
	
	/** Check if the run-time has been initialized. */
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
	if (name == NULL){
		return Nil;
	}

	c = objc_class_table_get(objc_classes, name);
	if (c == NULL || c->flags.in_construction){
		/* NULL, or still in construction */
		return Nil;
	}
	
	return c;
}


/***** IVAR-RELATED *****/
#pragma mark -
#pragma mark Ivar-related

Ivar objc_class_add_ivar(Class cls, const char *name, unsigned int size, unsigned int alignment, const char *types){
	Ivar variable;
	
	if (cls == Nil || name == NULL || size == 0 || types == NULL){
		return NULL;
	}
	
	if (!cls->flags.in_construction){
		objc_log("Class %s isn't in construction!\n", cls->name);
		objc_abort("Trying to add ivar to a class that isn't in construction.");
	}
	
	if (_ivar_named(cls, name) != NULL){
		objc_log("Class %s, or one of its superclasses already have an ivar named %s!\n", cls->name, name);
		return NULL;
	}
	
	// TODO lock
	if (cls->ivars == NULL){
		cls->ivars = objc_ivar_list_create(1);
	}else{
		cls->ivars = objc_ivar_list_expand_by(cls->ivars, 1);
	}
	
	variable = &cls->ivars->ivar_list[cls->ivars->size - 1];
	variable->name = objc_strcpy(name);
	variable->type = objc_strcpy(types);
	variable->size = size;
	variable->align = alignment;
	
	/* The offset is the aligned end of the instance size. */
	variable->offset = cls->instance_size;
	if (alignment != 0 && (variable->offset % alignment) > 0){
		variable->offset = (variable->offset + (alignment - 1)) & ~(alignment - 1);
	}
	
	cls->instance_size = variable->offset + size;
		
	return variable;
}
Ivar objc_class_get_ivar(Class cls, const char *name){
	return _ivar_named(cls, name);
}
Ivar *objc_class_get_ivar_list(Class cl){
	unsigned int number_of_ivars = _ivar_count(cl);
	Ivar *ivars = objc_alloc(sizeof(Ivar) * (number_of_ivars + 1));
	_ivars_copy_to_list(cl, ivars, number_of_ivars);
	return ivars;
}
Ivar objc_object_get_variable_named(id obj, const char *name, void **out_value){
	Ivar ivar;
	
	if (obj == nil || name == NULL || out_value == NULL){
		return NULL;
	}
	
	ivar = _ivar_named(obj->isa, name);
	if (ivar == NULL){
		return NULL;
	}
	
	objc_copy_memory((char*)obj + ivar->offset, *out_value, ivar->size);
	
	return ivar;
}
Ivar objc_object_set_variable_named(id obj, const char *name, void *value){
	Ivar ivar;
	
	if (obj == nil || name == NULL){
		return NULL;
	}
	
	ivar = _ivar_named(obj->isa, name);
	if (ivar == NULL){
		return NULL;
	}
	
	objc_copy_memory(value, (char*)obj + ivar->offset, ivar->size);
	
	return ivar;
}
void *objc_object_get_variable(id obj, Ivar ivar){
	char *var_ptr;
	
	if (obj == nil || ivar == NULL){
		return NULL;
	}
	
	var_ptr = (char*)obj;
	var_ptr += ivar->offset;
	return var_ptr;
}
void objc_object_set_variable(id obj, Ivar ivar, void *value){
	if (obj == nil || ivar == NULL){
		return;
	}
	
	objc_copy_memory(value, (char*)obj + ivar->offset, ivar->size);
}

/***** PROTOTYPE-RELATED *****/
#pragma mark -
#pragma mark Prototype-related

/*
Class objc_class_register_prototype(struct objc_class_prototype *prototype){
	Class cl;
	
	** Check if the run-time has been initialized. *
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
	objc_rw_lock_wlock(objc_runtime_lock);
	cl = _register_prototype(prototype);
	objc_rw_lock_unlock(objc_runtime_lock);
	return cl;
}
void objc_class_register_prototypes(struct objc_class_prototype *prototypes[]){
	unsigned int i = 0;
	
	** Check if the run-time has been initialized. *
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
	objc_rw_lock_wlock(objc_runtime_lock);
	while (prototypes[i] != NULL){
		_register_prototype(prototypes[i]);
		++i;
	}
	objc_rw_lock_unlock(objc_runtime_lock);
}
 */



/**** CACHE-RELATED ****/
#pragma mark -
#pragma mark Cache-related

/**
 * Flushing caches is a little tricky. As the structure is
 * read-lock-free, there can be multiple readers present.
 *
 * To solve this, the structure must handle this. The default
 * implementation solves this by simply marking the structure
 * as 'to be deallocated'. At the beginning of each read from
 * the cache, reader count is increased, at the end decreased.
 * 
 * Once the reader count is zero and the structure is marked
 * as to be deallocated, it is actually deallocated. This can be
 * done simply because the cache structure is marked to be
 * deleted *after* a new one has been created and placed instead.
 */

void objc_class_flush_cache(Class cl){
	if (cl == Nil){
		return;
	}
	
	// TODO - proper
	SparseArrayDestroy(cl->cache);
	cl->cache = NULL;
}

/***** INITIALIZATION *****/
#pragma mark -
#pragma mark Initializator-related

/**
 * Initializes the class extensions and internal class structures.
 */
void objc_class_init(void){
	objc_rw_lock_init(&objc_runtime_lock);
	
	objc_classes = objc_class_table_create(OBJC_CLASS_TABLE_INITIAL_CAPACITY);
	
	// TODO
	//objc_classes_array = objc_array_create();
}

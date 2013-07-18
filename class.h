/*
 * This header file contains declarations
 * of functions that deal with the class structure.
 */

#ifndef OBJC_CLASS_H_	 
#define OBJC_CLASS_H_

#include "types.h" /* For Class, BOOL, Method, ... definitions. */
#include "kernobjc/runtime.h"

/**
 * Two simple macros that determine whether the object is
 * a class or not. As the class->isa points to a class which
 * has a meta flag, it is fairly easy.
 */
#define OBJC_OBJ_IS_CLASS(obj) (obj->isa->flags.is_meta)
#define OBJC_OBJ_IS_INSTANCE(obj) (!OBJC_OBJ_IS_CLASS(obj))

#pragma mark -
#pragma mark Object creation, copying and destruction

/**
 * This function allocates enough space to contain an instance
 * of the class.
 */
extern id objc_class_create_instance(Class cl);

/**
 * Deallocates an instance.
 */
extern void objc_object_deallocate(id obj);

/**
 * Copies obj and returns the exact same copy.
 *
 * This copy is shallow.
 */
extern id objc_object_copy(id obj);


#pragma mark -
#pragma mark Object lookup

/**
 * The run-time automatically detects whether obj
 * is a class or an instance and handles the situation
 * accordingly.
 */
extern Method objc_object_lookup_method(id obj, SEL selector);
extern Method objc_object_lookup_method_super(struct objc_super *sup, SEL selector);
extern IMP objc_object_lookup_impl(id obj, SEL selector);
extern IMP objc_object_lookup_impl_super(struct objc_super *sup, SEL selector);


#pragma mark -
#pragma mark Information getters

/**
 * Returns YES if the class is resolved.
 * This generally means that objc_class_finish has been
 * called with this class, or the class was loaded from
 * an image and then resolved.
 */
extern BOOL objc_class_is_resolved(Class cl);




/**
 * Returns the meta class.
 */
Class objc_class_get_meta_class(const char *name);






#pragma mark -
#pragma mark Ivar-related

/**
 * Adds an ivar to a class. If the class is not in construction,
 * calling this function aborts the program.
 */
extern Ivar objc_class_add_ivar(Class cls, const char *name, unsigned int size, unsigned int alignment, const char *types);



/**
 * Returns the ivar as well as the value for the object.
 */
extern Ivar objc_object_get_variable_named(id obj, const char *name, void **out_value);

/**
 * Sets the ivar for that particular object.
 */
extern Ivar objc_object_set_variable_named(id obj, const char *name, void *value);


#define OBJC_SMALL_OBJECT_MASK ((sizeof(void*) == 4) ? 1 : 7)

Class objc_small_object_classes[7];

BOOL objc_register_small_object_class(Class cl, uintptr_t mask);


static inline Class objc_class_for_small_object(id obj){
	uintptr_t mask = ((uintptr_t)obj & OBJC_SMALL_OBJECT_MASK);
	// TODO unlikely
	if (mask != 0){
		if (sizeof(void*) == 4){
			// 32-bit system
			return objc_small_object_classes[0];
		}else{
			return objc_small_object_classes[mask];
		}
	}
	return Nil;
}

static inline BOOL objc_object_is_small_object(id obj){
	return objc_class_for_small_object(obj) != Nil;
}

__attribute__((always_inline)) static inline Class objc_object_get_class_inline(id obj){
	Class cl = objc_class_for_small_object(obj);
	// TODO unlikely
	if (cl != Nil){
		return cl;
	}
	
	return obj->isa;
}

__attribute__((always_inline)) static inline Class objc_class_get_nonfake_inline(Class cl){
	while (cl != Nil && cl->flags.fake) {
		cl = cl->super_class;
	}
	return cl;
}

__attribute__((always_inline)) static inline Class objc_object_get_nonfake_class_inline(id obj){
	Class cl = objc_object_get_class_inline(obj);
	cl = objc_class_get_nonfake_inline(cl);
	return cl;
}

#endif /* OBJC_CLASS_H_ */

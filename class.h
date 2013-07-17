/*
 * This header file contains declarations
 * of functions that deal with the class structure.
 */

#ifndef OBJC_CLASS_H_	 
#define OBJC_CLASS_H_

#include "types.h" /* For Class, BOOL, Method, ... definitions. */

/**
 * Two simple macros that determine whether the object is
 * a class or not. As the class->isa points to a class which
 * has a meta flag, it is fairly easy.
 */
#define OBJC_OBJ_IS_CLASS(obj) (obj->isa->flags.is_meta)
#define OBJC_OBJ_IS_INSTANCE(obj) (!(obj->isa->flags.is_meta))

#pragma mark -
#pragma mark Responding to selectors

/**
 * Returns YES if the class respons to the selector. This includes
 * the class' superclasses.
 */
extern BOOL objc_class_responds_to_selector(Class cl, SEL selector);


#pragma mark -
#pragma mark Lookup functions

extern Method objc_lookup_method(Class cl, SEL selector);
extern IMP objc_lookup_method_impl(Class cl, SEL selector);

#pragma mark -
#pragma mark Setting class of an object

/**
 * Sets the isa pointer of obj and returns the original
 * class.
 */
extern Class objc_object_set_class(id obj, Class new_class);


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
extern Method objc_object_lookup_method_super(objc_super *sup, SEL selector);
extern IMP objc_object_lookup_impl(id obj, SEL selector);
extern IMP objc_object_lookup_impl_super(objc_super *sup, SEL selector);


#pragma mark -
#pragma mark Information getters

/**
 * Returns YES if the class is resolved.
 * This generally means that objc_class_finish has been
 * called with this class, or the class was loaded from
 * an image and then resolved.
 */
extern BOOL objc_class_resolved(Class cl);

/**
 * Returns the name of the class.
 */
extern const char *objc_class_get_name(Class cl);

/**
 * Returns the superclass of cl, or Nil if it's a root class.
 */
extern Class objc_class_get_superclass(Class cl);

/**
 * Returns the class of the object, generally the isa pointer.
 */
extern Class objc_object_get_class(id obj);

/**
 * Returns size of an instance of a class.
 */
extern unsigned int objc_class_instance_size(Class cl);


/**
 * The following two functions return a list of methods
 * implemented on a class. The list if NULL-terminated
 * and the caller is responsible for freeing it using the
 * objc_dealloc function.
 *
 * Note that these functions only return methods that are
 * implemented directly on the class. Methods implemented
 * on the class' superclasses are not included.
 */
extern Method *objc_class_get_method_list(Class cl);



#pragma mark -
#pragma mark Ivar-related

/**
 * Adds an ivar to a class. If the class is not in construction,
 * calling this function aborts the program.
 */
extern Ivar objc_class_add_ivar(Class cls, const char *name, unsigned int size, unsigned int alignment, const char *types);

/**
 * Returns an ivar for name.
 */
extern Ivar objc_class_get_ivar(Class cls, const char *name);

/**
 * Returns a list of ivars. The list if NULL-terminated
 * and the caller is responsible for freeing it using the
 * objc_dealloc function.
 *
 * Note that this function only returns ivars that are
 * declared directly on the class. Ivars declared
 * on the class' superclasses are not included.
 */
extern Ivar *objc_class_get_ivar_list(Class cl);

/**
 * Returns the ivar as well as the value for the object.
 */
extern Ivar objc_object_get_variable_named(id obj, const char *name, void **out_value);

/**
 * Sets the ivar for that particular object.
 */
extern Ivar objc_object_set_variable_named(id obj, const char *name, void *value);

/**
 * Similar to previous functions, but faster if you
 * already have the Ivar pointer.
 */
extern void objc_object_set_variable(id obj, Ivar ivar, void *value);
extern void *objc_object_get_variable(id object, Ivar ivar);

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
	
	cl = obj->isa;
	while (cl != Nil && cl->flags.fake) {
		cl = cl->super_class;
	}
	
	return cl;
}

#endif /* OBJC_CLASS_H_ */

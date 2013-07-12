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
#pragma mark Adding methods

/**
 * The following functions add methods or a single method to the class.
 *
 * If a method with the same selector is already attached to the class,
 * this doesn't override it. This is due to the method lists being a linked
 * list and the new methods being attached to the end of the list
 * as well as maintaining behavior of other run-times.
 *
 * Caller is responsible for the method 'objects' to be kept in memory,
 * however, the method array does get copied. In particular, when you
 * call the add_*_methods function, you are responsible for each *m,
 * however, you may free(m) when done.
 */
extern void objc_class_add_method(Class cl, Method m);
extern void objc_class_add_methods(Class cl, Method *m, unsigned int count);


#pragma mark -
#pragma mark Replacing methods

/**
 * Replaces a method implementation for another one.
 *
 * Returns the old implementation.
 */
extern IMP objc_class_replace_method_implementation(Class cls, SEL name, IMP imp, const char *types);






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
 * Returns YES if the class is currently in construction.
 * This generally means that objc_class_finish hasn't been
 * called with this class yet.
 */
extern BOOL objc_class_in_construction(Class cl);

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


__attribute__((always_inline)) static inline Class objc_object_get_class_inline(id obj){
	// TODO check for in-pointer classes
	return obj->isa;
}

#pragma mark -
#pragma mark Cache-related

/**
 * These cache-related functions flush the caches of a class,
 * which then requires the lookup function to search for the
 * method implementation once again.
 *
 */
extern void objc_class_flush_cache(Class cl);

#endif /* OBJC_CLASS_H_ */

/*
 * This header file contains declarations
 * of functions that deal with the class structure.
 */

#ifndef OBJC_CLASS_H_	 
#define OBJC_CLASS_H_

#include "types.h" /* For Class, BOOL, Method, ... definitions. */

/**
 * Two simple macros that determine whether the object is
 * a class or not. As the class->isa pointer creates a loop to
 * itself, the detection is very easy.
 */
#define OBJC_OBJ_IS_CLASS(obj) ((BOOL)(obj->isa == (Class)obj))
#define OBJC_OBJ_IS_INSTANCE(obj) ((BOOL)(obj->isa != (Class)obj))

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
extern void objc_class_add_class_method(Class cl, Method m);
extern void objc_class_add_class_methods(Class cl, Method *m, unsigned int count);
extern void objc_class_add_instance_method(Class cl, Method m);
extern void objc_class_add_instance_methods(Class cl, Method *m, unsigned int count);


#pragma mark -
#pragma mark Replacing methods

/**
 * Replaces a method implementation for another one.
 *
 * Returns the old implementation.
 */
extern IMP objc_class_replace_instance_method_implementation(Class cls, SEL name, IMP imp, const char *types);
extern IMP objc_class_replace_class_method_implementation(Class cls, SEL name, IMP imp, const char *types);


#pragma mark -
#pragma mark Creating classes

/**
  * Creates a new class with name that is a subclass of superclass.
  * 
  * The returned class isn't registered with the run-time yet,
  * you need to use the objc_registerClass function.
  *
  * Memory management note: the name is copied.
  */
extern Class objc_class_create(Class superclass, const char *name);

/**
 * This function marks the class as finished (i.e. not in construction)
 * and calls class initializers of all class extensions on that class.
 * You need to call this before creating any instances.
 */
extern void objc_class_finish(Class cl);



#pragma mark -
#pragma mark Responding to selectors

/**
 * Returns YES if the class respons to the selector. This includes
 * the class' superclasses.
 */
extern BOOL objc_class_responds_to_instance_selector(Class cl, SEL selector);
extern BOOL objc_class_responds_to_class_selector(Class cl, SEL selector);



#pragma mark -
#pragma mark Regular lookup functions

/**
 * The following functions lookup a method on a class, or an object.
 *
 * Each class has two caches - one for instances, one for classes.
 * The run-time first looks into the appropriate cache - if a Method
 * object is found, it is returned (or the IMP).
 *
 * If the method is not found, the class extension lookup functions
 * are called (this is so that the extensions may override methods,
 * allowing categories, proxies, etc.).
 *
 * If the class extensions do not find the method, the class hierarchy
 * is climbed and searched for such a method.
 *
 * Once a method is found, at any point of the lookup, it is cached for
 * later use.
 *
 * If, for example, a class extension changes in a way that the lookup would
 * result in a different method implementation, it is responsible to clear
 * the class caches (see the cache-related functions below).
 *
 * NOTE: The functions looking up method *implementation* NEVER
 * return NULL! If cl or obj are Nil (or nil), a no-op function is returned.
 * If the method is not found, the class is then searched for a selector
 * -(BOOL)forwardMessage:(SEL)selector; and the class or the object is
 * then called with this method. If the class doesn't implement
 * the forwardMessage: method, or the method returns NO,
 * the program is aborted. If it returns YES, a no-op function pointer
 * is returned.
 *
 * This is a simplified forwarding mechanism with less overhead. In case
 * the forwarding mechanism used by Apple and others is needed for
 * compatibility reasons, it can be easily implemented within this method.
 */
extern Method objc_lookup_class_method(Class cl, SEL selector);
extern IMP objc_lookup_class_method_impl(Class cl, SEL selector);
extern Method objc_lookup_instance_method(id obj, SEL selector);
extern IMP objc_lookup_instance_method_impl(id obj, SEL selector);


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
 * of the class, including the space required by class extensions.
 *
 * Each class extension gets called, if it implements an object
 * initializer function.
 *
 * If you do not use this function for object creation, you need to
 * call the complete_object function (see below) on each created object.
 */
extern id objc_class_create_instance(Class cl);

/**
 * If, for whatever reason, you do not use the run-time function
 * to create an instance (see above), always *do* call the finalize function
 * in order for class extensions to work. This function just calls
 * the object initializers of class extensions.
 *
 * Note that the preferred way of custom object allocation is via
 * the class extensions.
 */
extern void objc_complete_object(id instance);

/**
 * Deallocates an instance and calls all object_deallocator
 * functions of class extensions.
 *
 * If, again, for whatever reason, you are not using
 * the run-time functions, to create and deallocate
 * objects, use the function below so that the class
 * extension object_deallocator functions are called.
 */
extern void objc_object_deallocate(id obj);

/**
 * If, for whatever reason, you do not use the run-time function
 * to deallocate the object, you must call this function so that
 * all class extensions can deallocate anything they might have
 * allocated.
 *
 * Note that the preferred way of custom object allocation is via
 * the class extensions.
 */
extern void objc_finalize_object(id instance);

/**
 * Copies obj and returns the exact same copy.
 *
 * This copy is shallow.
 *
 * Note that this copies over even the class extension
 * data. If this is not preferred, call objc_complete_object
 * for the extension initializers to be called.
 */
extern id objc_object_copy(id obj);


#pragma mark -
#pragma mark Object lookup

/**
 * These functions differ from the previous ones that they are preferred to be
 * used by the compiler. The run-time automatically detects whether obj 
 * is a class or an instance and handles the situation depending on that.
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
 * Returns size of an instance of a class. This size does include
 * the space required by class extensions.
 */
extern unsigned int objc_class_instance_size(Class cl);

/**
 * Returns a list of classes registered with the run-time. The list
 * is NULL-terminated and the caller is responsible for freeing
 * it using the objc_dealloc function.
 */
extern Class *objc_class_get_list(void);

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
extern Method *objc_class_get_instance_method_list(Class cl);
extern Method *objc_class_get_class_method_list(Class cl);

/**
 * Finds a class registered with the run-time and returns it,
 * or Nil, if no such class is registered.
 *
 * Note that if the class is currently being in construction,
 * Nil is returned anyway.
 */
extern Class objc_class_for_name(const char *name);


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


#pragma mark -
#pragma mark Prototype-related

/**
 * This is a way to register a class prototype. The protoype must:
 *
 * - have ->isa == NULL. It gets connected in this method.
 * - name must be non-NULL.
 * - instance_methods and class_methods fields must be
 *	struct objc_method_prototype **, NULL-terminated lists,
 *	ivars in a NULL-terminated Ivar * list.
 * - must be in construction.
 *
 * All lists passed must be NULL-terminated. The run-time
 * registers all selectors, modifies the method prototypes to
 * Methods.
 *
 * Returned value is either Nil, if such a class already exists,
 * or the same pointer as the prototype. (All modifications are
 * in-place.)
 */
struct objc_class_prototype;
extern Class objc_class_register_prototype(struct objc_class_prototype *prototype);
extern void objc_class_register_prototypes(struct objc_class_prototype *prototypes[]);


#pragma mark -
#pragma mark Cache-related

/**
 * These cache-related functions flush the caches of a class,
 * which then requires the lookup function to search for the
 * method implementation once again.
 *
 * These functions should be used by class extensions in case
 * they have loaded new methods, or similar, so that the old cached
 * method implementations aren't returned.
 */
extern void objc_class_flush_caches(Class cl);
extern void objc_class_flush_instance_cache(Class cl);
extern void objc_class_flush_class_cache(Class cl);

#endif /* OBJC_CLASS_H_ */

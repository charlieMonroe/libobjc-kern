
#ifndef OBJC_CLASS_REGISTRY_H
#define OBJC_CLASS_REGISTRY_H


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
 * This function marks the class as finished (i.e. not in construction).
 * You need to call this before creating any instances.
 */
extern void objc_class_finish(Class cl);

/**
 * Returns a list of classes registered with the run-time. The list
 * is NULL-terminated and the caller is responsible for freeing
 * it using the objc_dealloc function.
 */
extern Class *objc_class_get_list(void);

/**
 * Finds a class registered with the run-time and returns it,
 * or Nil, if no such class is registered.
 *
 * Note that if the class is currently being in construction,
 * Nil is returned anyway.
 */
extern Class objc_class_for_name(const char *name);

/**
 * Registers count of classes with the runtime.
 *
 * If resolve_super is YES, it will treat the super_class
 * pointer as const char * pointer and will try to resolve
 * the class name.
 */
extern void objc_register_classes(Class *classes, int count, BOOL resolve_super);


#endif // OBJC_CLASS_REGISTRY_H

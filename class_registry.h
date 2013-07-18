
#ifndef OBJC_CLASS_REGISTRY_H
#define OBJC_CLASS_REGISTRY_H

#include "types.h"

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
 * Registers a class with the runtime.
 *
 * Used for loading classes from a binary image.
 * Assumes runtime lock is locked.
 */
extern void objc_class_register_class(Class cl);
extern void objc_class_register_classes(Class *cl, unsigned int count);

/**
 * TODO
 */
PRIVATE BOOL objc_class_resolve(Class cl);


#endif // OBJC_CLASS_REGISTRY_H

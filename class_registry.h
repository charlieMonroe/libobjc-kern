
#ifndef OBJC_CLASS_REGISTRY_H
#define OBJC_CLASS_REGISTRY_H

#include "types.h"

/**
 * Registers a class with the runtime.
 *
 * Used for loading classes from a binary image.
 * Assumes runtime lock is locked.
 */
extern void objc_class_register_class(Class cl);
extern void objc_class_register_classes(Class *cl, unsigned int count);

PRIVATE BOOL objc_class_resolve(Class cl);

PRIVATE void objc_updateDtableForClassContainingMethod(Method m);

#endif // OBJC_CLASS_REGISTRY_H

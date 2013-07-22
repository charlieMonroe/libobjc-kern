
#ifndef OBJC_CLASS_REGISTRY_H
#define OBJC_CLASS_REGISTRY_H

#include "types.h"

/*
 * Registers a class with the runtime.
 *
 * Used for loading classes from a binary image.
 * Assumes runtime lock is locked.
 */
void		objc_class_register_class(Class cl);
void		objc_class_register_classes(Class *cl, unsigned int count);


#endif // OBJC_CLASS_REGISTRY_H

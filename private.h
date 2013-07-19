
#ifndef OBJC_PRIVATE_H
#define OBJC_PRIVATE_H

/**
 * These functions are either marked as unavailble with ARC,
 * or indeed private.
 */

id class_createInstance(Class cl, size_t extraBytes);

id object_copy(id obj, size_t size);
void object_dispose(id obj);

void *object_getIndexedIvars(id obj);

BOOL objc_register_small_object_class(Class cl, uintptr_t mask);

#endif

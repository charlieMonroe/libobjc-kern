
#ifndef OBJC_PRIVATE_H
#define OBJC_PRIVATE_H

/**
 * These functions are either marked as unavailble with ARC,
 * or indeed private.
 */

#pragma mark -
#pragma mark ARC_UNAVAILABLE

id class_createInstance(Class cl, size_t extraBytes);
id object_copy(id obj, size_t size);
void object_dispose(id obj);
void *object_getIndexedIvars(id obj);


#pragma mark -
#pragma mark Private

PRIVATE BOOL	objc_register_small_object_class(Class cl, uintptr_t mask);
PRIVATE Slot	objc_class_get_slot(Class cl, SEL selector);
PRIVATE BOOL	objc_class_resolve(Class cl);
PRIVATE void	objc_updateDtableForClassContainingMethod(Method m);

#endif

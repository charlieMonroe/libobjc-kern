
#ifndef OBJC_ASSOCIATIVE_H
#define OBJC_ASSOCIATIVE_H

#include "types.h"


/**
 * Returns an object previously stored by calling objc_set_associated_object()
 * with the same arguments, or nil if none exists.
 */
id objc_get_associated_object(id object, void *key);

/**
 * Associates an object with another.  This provides a mechanism for storing
 * extra state with an object, beyond its declared instance variables.  The
 * pointer used as a key is treated as an opaque value.  The best way of
 * ensuring this is to pass the pointer to a static variable as the key.  The
 * value may be any object, but must respond to -copy or -retain, and -release,
 * if an association policy of copy or retain is passed as the final argument.
 */
void objc_set_associated_object(id object, void *key, id value, objc_AssociationPolicy policy);

/**
 * Removes all associations from an object.
 */
void objc_remove_associated_objects(id object);

/**
 * Removes and zeroes out all weak refs and removes them from the associated
 * objects.
 */
void objc_remove_associated_weak_refs(id object);


#endif // OBJC_ASSOCIATIVE_H


#ifndef OBJC_ASSOCIATIVE_H
#define OBJC_ASSOCIATIVE_H

#include "types.h"

/**
 * Valid values for objc_AssociationPolicy.  This is really a bitfield, but
 * only specific combinations of flags are permitted.
 */
enum
{
	/**
	 * Perform straight assignment, no message sends.
	 */
	OBJC_ASSOCIATION_ASSIGN = 0,
	/**
	 * Retain the associated object.
	 */
	OBJC_ASSOCIATION_RETAIN_NONATOMIC = 1,
	/**
	 * Copy the associated object, by sending it a -copy message.
	 */
	OBJC_ASSOCIATION_COPY_NONATOMIC = 3,
	/**
	 * Atomic retain.
	 */
	OBJC_ASSOCIATION_RETAIN = 0x301,
	/**
	 * Atomic copy.
	 */
	OBJC_ASSOCIATION_COPY = 0x303
};
/**
 * Association policy, used when setting associated objects.
 */
typedef uintptr_t objc_association_policy;


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
void objc_set_associated_object(id object, void *key, id value, objc_association_policy policy);

/**
 * Removes all associations from an object.
 */
void objc_remove_associated_objects(id object);




#endif // OBJC_ASSOCIATIVE_H


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
typedef uintptr_t objc_AssociationPolicy;




#endif // OBJC_ASSOCIATIVE_H


#ifndef OBJC_LIST_TYPES_H
#define OBJC_LIST_TYPES_H

/**
 * This file includes the list.h file multiple times,
 * each time with different options, hence creating
 * all the necessary list structures throughout the 
 * runtime, such as objc_method_list, etc.
 */

/** Method list. */
#define OBJC_LIST_TYPE_NAME method
#define OBJC_LIST_TYPE Method
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

/** Ivar list. */
#define OBJC_LIST_TYPE_NAME ivar
#define OBJC_LIST_TYPE Ivar
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/** Categories. */
#define OBJC_LIST_TYPE_NAME category
#define OBJC_LIST_TYPE Category
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/** Protocols. */
#define OBJC_LIST_TYPE_NAME protocol
#define OBJC_LIST_TYPE Protocol
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

/** Properties. */
#define OBJC_LIST_TYPE_NAME property
#define OBJC_LIST_TYPE Property
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

#endif // OBJC_LIST_TYPES_H

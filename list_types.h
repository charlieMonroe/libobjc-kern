
#ifndef OBJC_LIST_TYPES_H
#define OBJC_LIST_TYPES_H

/*
 * This file includes the list.h file multiple times,
 * each time with different options, hence creating
 * all the necessary list structures throughout the 
 * runtime, such as objc_method_list, etc.
 */

/** Method list. */
#define OBJC_LIST_TYPE_NAME method
#define OBJC_LIST_TYPE struct objc_method
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

/** Ivar list. */
#define OBJC_LIST_TYPE_NAME ivar
#define OBJC_LIST_TYPE struct objc_ivar
#define OBJC_LIST_STRUCTURE_CUSTOM_FREE_BLOCK(x) {			\
	objc_dealloc((void*)x->type);					\
	objc_dealloc((void*)x->name);					\
}
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/** Categories. */
#define OBJC_LIST_TYPE_NAME category
#define OBJC_LIST_TYPE struct objc_category
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/** Method description list */
#define OBJC_LIST_TYPE_NAME method_description
#define OBJC_LIST_TYPE struct objc_method_description
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/** Protocols. */
#define OBJC_LIST_TYPE_NAME protocol
#define OBJC_LIST_TYPE struct objc_protocol *
#define OBJC_LIST_VALUES_ARE_POINTERS 1
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

/** Properties. */
#define OBJC_LIST_TYPE_NAME property
#define OBJC_LIST_TYPE struct objc_property
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

#endif /* !OBJC_LIST_TYPES_H */

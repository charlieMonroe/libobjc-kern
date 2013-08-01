
#ifndef OBJC_LIST_TYPES_H
#define OBJC_LIST_TYPES_H

/*
 * This file includes the list.h file multiple times,
 * each time with different options, hence creating
 * all the necessary list structures throughout the 
 * runtime, such as objc_method_list, etc.
 */

/* Method list. */
#define OBJC_LIST_TYPE_NAME method
#define OBJC_LIST_TYPE struct objc_method
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

/* Ivar list. */

static MALLOC_DECLARE(M_IVAR_TYPE);
static MALLOC_DEFINE(M_IVAR_TYPE, "ivar list", "Objective-C Ivar List");
#define OBJC_LIST_MALLOC_TYPE M_IVAR_TYPE
#define OBJC_LIST_TYPE_NAME ivar
#define OBJC_LIST_TYPE struct objc_ivar
#define OBJC_LIST_STRUCTURE_CUSTOM_FREE_BLOCK(x) {                            \
	objc_dealloc((void*)x->type, M_LIST_TYPE);                                  \
	objc_dealloc((void*)x->name, M_LIST_TYPE);                                  \
}
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/* Categories. */
static MALLOC_DECLARE(M_CATEGORY_TYPE);
static MALLOC_DEFINE(M_CATEGORY_TYPE, "category list",
                     "Objective-C Category List");
#define OBJC_LIST_MALLOC_TYPE M_CATEGORY_TYPE
#define OBJC_LIST_TYPE_NAME category
#define OBJC_LIST_TYPE struct objc_category
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/* Method description list */
static MALLOC_DECLARE(M_METHOD_DESC_TYPE);
static MALLOC_DEFINE(M_METHOD_DESC_TYPE, "method description list",
                     "Objective-C Method Description List");
#define OBJC_LIST_MALLOC_TYPE M_METHOD_DESC_TYPE
#define OBJC_LIST_TYPE_NAME method_description
#define OBJC_LIST_TYPE struct objc_method_description
#define OBJC_LIST_CHAINABLE 0
#include "list.h"

/* Protocols. */
static MALLOC_DECLARE(M_PROTOCOL_TYPE);
static MALLOC_DEFINE(M_PROTOCOL_TYPE, "protocol list",
                     "Objective-C Protocol List");
#define OBJC_LIST_MALLOC_TYPE M_PROTOCOL_TYPE
#define OBJC_LIST_TYPE_NAME protocol
#define OBJC_LIST_TYPE struct objc_protocol *
#define OBJC_LIST_VALUES_ARE_POINTERS 1
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

/* Properties. */
static MALLOC_DECLARE(M_PROPERTY_TYPE);
static MALLOC_DEFINE(M_PROPERTY_TYPE, "property list",
                     "Objective-C Property List");
#define OBJC_LIST_MALLOC_TYPE M_PROPERTY_TYPE
#define OBJC_LIST_TYPE_NAME property
#define OBJC_LIST_TYPE struct objc_property
#define OBJC_LIST_CHAINABLE 1
#include "list.h"

#endif /* !OBJC_LIST_TYPES_H */

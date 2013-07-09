
/** No guards as we want this to be reincluded multiple times. */

#include "os.h" // Needed for some macros

/**
 * This header defines a template for a list
 * that can be chained using the ->next variable
 * if OBJC_LIST_CHAINABLE is defined as 1.
 *
 * You need to specify OBJC_LIST_TYPE_NAME
 * which is then used throughout the structure names
 * as well as function names.
 *
 * You can also define OBJC_LIST_TYPE which
 * defines the type of the array. If not defined,
 * falls back to (void *).
 */


#ifndef OBJC_LIST_TYPE_NAME
	#error You need to specify OBJC_LIST_TYPE_NAME.
#endif

#ifndef OBJC_LIST_TYPE
	#define OBJC_LIST_TYPE (void *)
#endif

#ifndef OBJC_LIST_CHAINABLE
	#define OBJC_LIST_CHAINABLE 0
#endif



/**
 * These macros define the structure name.
 * See the example below for better understanding.
 */
#define OBJC_LIST_STRUCTURE_TYPE_NAME \
	PREFIX_SUFFIX(\
		PREFIX_SUFFIX(objc_, OBJC_LIST_TYPE_NAME),\
		_list\
	)

#define OBJC_LIST_STRUCTURE_NAME \
	PREFIX_SUFFIX(\
		OBJC_LIST_STRUCTURE_TYPE_NAME, \
		_struct\
	)

/**
 * The list structure.
 *
 * Example:
 *
 * OBJC_LIST_TYPE_NAME = method
 * OBJC_LIST_TYPE = Method
 * OBJC_LIST_CHAINABLE = 1
 *
 * typedef struct objc_method_list_struct {
 *	struct objc_method_list_struct *next;
 *	unsigned size;
 *	Method *method_list;
 * }
 */
typedef struct OBJC_LIST_STRUCTURE_NAME {
	/**
	 * If chainable, include a pointer to the next
	 * structure.
	 */
	#if OBJC_LIST_CHAINABLE
		struct OBJC_LIST_STRUCTURE_NAME *next;
	#endif
	
	/**
	 * Size of the *_list below.
	 */
	unsigned int size;
	
	/**
	 * Actual list of the items.
	 */
	OBJC_LIST_TYPE *PREFIX_SUFFIX(OBJC_LIST_TYPE_NAME, _list);
} OBJC_LIST_STRUCTURE_TYPE_NAME;



#undef OBJC_LIST_TYPE_NAME
#undef OBJC_LIST_TYPE
#undef OBJC_LIST_CHAINABLE


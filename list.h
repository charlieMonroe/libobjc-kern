
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
struct OBJC_LIST_STRUCTURE_NAME {
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
	OBJC_LIST_TYPE PREFIX_SUFFIX(OBJC_LIST_TYPE_NAME, _list)[];
};

/**
 * Creates a new structure (the above one).
 */
static inline OBJC_LIST_STRUCTURE_TYPE_NAME *PREFIX_SUFFIX(OBJC_LIST_STRUCTURE_TYPE_NAME, _create)(unsigned int count){
	OBJC_LIST_STRUCTURE_TYPE_NAME *result = objc_zero_alloc(sizeof(OBJC_LIST_STRUCTURE_TYPE_NAME) + count * sizeof(OBJC_LIST_TYPE));
	result->size = count;
	return result;
}

static inline OBJC_LIST_STRUCTURE_TYPE_NAME *PREFIX_SUFFIX(OBJC_LIST_STRUCTURE_TYPE_NAME, _expand_by)(OBJC_LIST_STRUCTURE_TYPE_NAME *list, unsigned int count){
	list = objc_realloc(list, sizeof(OBJC_LIST_STRUCTURE_TYPE_NAME) + (list->size + count) * sizeof(OBJC_LIST_TYPE) );
	list->size += count;
	return list;
}

#if OBJC_LIST_CHAINABLE
/**
 * Appends a structure to the end of the list.
 */
static inline OBJC_LIST_STRUCTURE_TYPE_NAME *PREFIX_SUFFIX(OBJC_LIST_STRUCTURE_TYPE_NAME, _append)(
	OBJC_LIST_STRUCTURE_TYPE_NAME *head,
        OBJC_LIST_STRUCTURE_TYPE_NAME *to_append
){
	OBJC_LIST_STRUCTURE_TYPE_NAME *str = head;
	while (str->next != NULL){
		str = str->next;
	}
	str->next = to_append;
	return head;
}

static inline OBJC_LIST_STRUCTURE_TYPE_NAME *PREFIX_SUFFIX(OBJC_LIST_STRUCTURE_TYPE_NAME, _prepend)(
									 OBJC_LIST_STRUCTURE_TYPE_NAME *head,
									 OBJC_LIST_STRUCTURE_TYPE_NAME *to_prepend
									 ){
	to_prepend->next = head;
	return to_prepend;
}

/**
 * Goes through the whole linked list and copies all the ->*_list items into one big list.
 */
static inline OBJC_LIST_TYPE **PREFIX_SUFFIX(OBJC_LIST_STRUCTURE_TYPE_NAME, _flatten)(
												    OBJC_LIST_STRUCTURE_TYPE_NAME *head
												    ){
	// First, figure out how much to allocate.
	size_t size = 0;
	OBJC_LIST_STRUCTURE_TYPE_NAME *list = head;
	while (list != NULL){
		size += list->size;
		list = list->next;
	}
	
	// NULL-terminated
	OBJC_LIST_TYPE **objs = objc_alloc((size + 1) * sizeof(OBJC_LIST_TYPE*));
	unsigned int counter = 0;
	list = head;
	while (list != NULL && counter < size){
		for (int i = 0; i < list->size; ++i){
			objs[counter] = &(list->PREFIX_SUFFIX(OBJC_LIST_TYPE_NAME, _list)[i]);
			++counter;
			
			if (counter >= size){
				// Someone probably added something in the meanwhile
				break;
			}
		}
		
		list = list->next;
	}
	objs[size] = NULL; // NULL-terminate
	return objs;
}

#endif // OBJC_LIST_CHAINABLE

#undef OBJC_LIST_TYPE_NAME
#undef OBJC_LIST_TYPE
#undef OBJC_LIST_CHAINABLE


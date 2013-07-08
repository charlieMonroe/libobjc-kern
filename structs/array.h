/*
  * Default objc_array implementation.
  */
  
#ifndef OBJC_ARRAY_H_
#define OBJC_ARRAY_H_

#include "../os.h"
#if !OBJC_USES_INLINE_FUNCTIONS

#include "../types.h"

/* Creates a new objc_array object. */
extern objc_array array_create(void);

/**
 * Destroys (i.e. frees the objc_array object). No action is
 * performed on the pointers. It's up to the array owner
 * to destroy them.
 */
extern void array_destroy(objc_array array);

/* Add pointer to the array. */
extern void array_add(objc_array array, void *ptr);

objc_array_enumerator array_get_enumerator(objc_array array);

#endif /* OBJC_USES_INLINE_FUNCTIONS */

#endif /* OBJC_ARRAY_H_ */

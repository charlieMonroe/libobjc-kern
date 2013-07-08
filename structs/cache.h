
#ifndef CACHE_H_
#define CACHE_H_

#include "../os.h"
#if !OBJC_USES_INLINE_FUNCTIONS

	#include "../types.h"

	extern objc_cache cache_create(void);
	extern Method cache_fetch(objc_cache cache, SEL selector);
	extern void cache_destroy(objc_cache cache);
	extern void cache_insert(objc_cache cache, Method method);

#endif /* OBJC_USES_INLINE_FUNCTIONS */

#endif /* CACHE_H_ */

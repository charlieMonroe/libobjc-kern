
#ifndef OBJC_OS_H_
#define OBJC_OS_H_

/* Constants */
#define OBJC_DEBUG_LOG 1

#define KERNEL_OBJC 1

/* Compiler attributes and directives. */
#define UNREACHABLE(x) __builtin_unreachable()
#define PRIVATE  __attribute__ ((visibility("hidden")))
#define LIKELY(x) __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0)


/* Preprocessor magic */
#define REALLY_PREFIX_SUFFIX(x, y) x ## y
#define PREFIX_SUFFIX(x, y) REALLY_PREFIX_SUFFIX(x, y)

/* For statistics. */
PRIVATE extern unsigned int objc_lock_count;
PRIVATE extern unsigned int objc_lock_destroy_count;
PRIVATE extern unsigned int objc_lock_locked_count;

#ifndef __has_feature         // Optional of course.
  #define __has_feature(x) 0  // Compatibility with non-clang compilers.
#endif

#if !__has_feature(objc_arc)
#define __unsafe_unretained
#endif

/* Include of the relevant header. */
#ifndef _KERNEL
	#include "os/user.h"
#else /* _KERNEL */
	#include "os/kernel.h"
#endif /* !_KERNEL */

#include "malloc_types.h"

#define objc_format_string snprintf

#define objc_assert(condition, description...)				\
	if (!(condition)){						\
		objc_abort(description);				\
	}


/* Locking macros. */
#define OBJC_LOCK(x) objc_rw_lock_wlock(x)
#define OBJC_UNLOCK(x) objc_rw_lock_unlock(x)

__attribute__((unused)) static
void objc_release_lock(void *x)
{
	objc_rw_lock *lock = *(objc_rw_lock**)x;
	OBJC_UNLOCK(lock);
}

#define OBJC_LOCK_FOR_SCOPE(lock)					\
	__attribute__((cleanup(objc_release_lock)))			\
	__attribute__((unused)) objc_rw_lock *lock_pointer = lock;	\
	OBJC_LOCK(lock)



#endif /* !OBJC_OS_H_ */

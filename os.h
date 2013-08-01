
#ifndef OBJC_OS_H_
#define OBJC_OS_H_

/* Constants */
#define OBJC_DEBUG_LOG 1

/* Compiler attributes and directives. */
#define UNREACHABLE(x) __builtin_unreachable()
#define PRIVATE  __attribute__ ((visibility("hidden")))
#define LIKELY(x) __builtin_expect(x, 1)
#define UNLIKELY(x) __builtin_expect(x, 0)


/* Preprocessor magic */
#define REALLY_PREFIX_SUFFIX(x, y) x ## y
#define PREFIX_SUFFIX(x, y) REALLY_PREFIX_SUFFIX(x, y)

/* For statistics. */
PRIVATE unsigned int objc_lock_count;
PRIVATE unsigned int objc_lock_destroy_count;
PRIVATE unsigned int objc_lock_locked_count;

#ifndef __has_feature         // Optional of course.
  #define __has_feature(x) 0  // Compatibility with non-clang compilers.
#endif

#if !__has_feature(objc_arc)
#define __unsafe_unretained
#endif

#define _KERNEL

/* Include of the relevant header. */
#ifndef _KERNEL
	#include "os/user.h"
#else /* _KERNEL */
	#include "os/kernel.h"
#endif /* !_KERNEL */

#include "malloc_types.h"

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

#define OBJC_LOCK_OBJECT_FOR_SCOPE(obj)					\
	__attribute__((cleanup(objc_release_object_lock)))		\
	__attribute__((unused)) id lock_object_pointer = obj;		\
	objc_sync_enter(obj);



#endif /* !OBJC_OS_H_ */

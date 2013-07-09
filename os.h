/**
 * This header file contains various declarations of functions.
 */

#ifndef OBJC_OS_H_
#define OBJC_OS_H_

#include <stdio.h>

#define OBJC_INLINE static inline

/**
 * Some compilers might not support this attribute
 * that forces the compiler to always inline the functions.
 */
// Not to be used in this runtime, to be removed later
#define OBJC_ALWAYS_INLINE __attribtue__((deprecated))__

// TODO
static inline void panic(const char *reason){
	printf("%s", reason);
	exit(1);
}

#define objc_assert(condition, description) \
		if (!(condition)){\
			panic(description);\
		}

// TODO
#define objc_lock void
#define objc_array void

/**
 * Some small macros that are used in a few places,
 * mostly for stitching.
 */
#define REALLY_PREFIX_SUFFIX(x, y) x ## y
#define PREFIX_SUFFIX(x, y) REALLY_PREFIX_SUFFIX(x, y)

#endif /* OBJC_OS_H_ */

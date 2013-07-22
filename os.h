/**
 * This header file contains various declarations of functions.
 */

#ifndef OBJC_OS_H_
#define OBJC_OS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

/**
 * Some compilers might not support this attribute
 * that forces the compiler to always inline the functions.
 */
// Not to be used in this runtime, to be removed later

#define PAGE_SIZE 4096

#define UNREACHABLE(x) __builtin_unreachable()
#define PUBLIC __attribute__ ((visibility("default")))
#define PRIVATE  __attribute__ ((visibility("hidden")))


#define OBJC_DEBUG_LOG 1

#define objc_debug_log(...) \
	if (OBJC_DEBUG_LOG) { printf("DEBUG: "); printf(__VA_ARGS__); }


// TODO
#define objc_lock void
#define objc_array void
typedef struct {
	// TODO
} objc_rw_lock;

#define objc_log printf

static inline void objc_dealloc(void *mem){
	free(mem);
}

// TODO
#define panic(reason...) {\
	printf(reason);\
	abort();\
}

#define objc_abort(reason...) {\
	panic(reason);\
}

static inline void *objc_realloc(void *mem, size_t size){
	return realloc(mem, size);
}

static inline void *objc_zero_alloc(size_t size){
	return calloc(1, size);
}

static inline void *objc_alloc(size_t size){
	return malloc(size);
}

static inline void *objc_alloc_page(void){
	// TODO use page allocator directly?
	return malloc(PAGE_SIZE);
}

static inline void objc_rw_lock_init(objc_rw_lock *lock){
	// TODO
}
static inline int objc_rw_lock_rlock(objc_rw_lock *lock){
	return 0;
}
static inline int objc_rw_lock_wlock(objc_rw_lock *lock){
	return 0;
}
static inline int objc_rw_lock_unlock(objc_rw_lock *lock){
	return 0;
}
static inline int objc_rw_lock_destroy(objc_rw_lock *lock){
	return 0;
}

#define OBJC_LOCK(x) objc_rw_lock_wlock(x)
#define OBJC_UNLOCK(x) objc_rw_lock_unlock(x)


__attribute__((unused)) static void objc_release_lock(void *x){
	objc_rw_lock *lock = *(objc_rw_lock**)x;
	OBJC_UNLOCK(lock);
}

#define OBJC_LOCK_FOR_SCOPE(lock) \
		__attribute__((cleanup(objc_release_lock)))\
		__attribute__((unused)) objc_rw_lock *lock_pointer = lock;\
		OBJC_LOCK(lock)

#define OBJC_LOCK_OBJECT_FOR_SCOPE(obj) \
		__attribute__((cleanup(objc_release_object_lock)))\
		__attribute__((unused)) id lock_object_pointer = obj;\
		objc_sync_enter(obj);

#define objc_assert(condition, description...) \
		if (!(condition)){\
			panic(description);\
		}



/**
 * Some small macros that are used in a few places,
 * mostly for stitching.
 */
#define REALLY_PREFIX_SUFFIX(x, y) x ## y
#define PREFIX_SUFFIX(x, y) REALLY_PREFIX_SUFFIX(x, y)

#endif /* OBJC_OS_H_ */


#ifndef _INLINE_FUNCTIONS_SAMPLE_H_
#define _INLINE_FUNCTIONS_SAMPLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "../types.h"

/**
 * Inline functions. Defines should be avoided in most cases.
 */

/**
 * Using a define to simplify dealing with the variadic function parameters.
 *
 * The declarations here, however, should indeed be functions as they
 * may be referenced.
 */
#define objc_log printf

OBJC_INLINE void objc_abort(const char *reason) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_abort(const char *reason){
	printf("Aborting because of %s.", reason);
	abort();
}

OBJC_INLINE void *objc_alloc(unsigned long size) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *objc_alloc(unsigned long size){
	return malloc(size);
}

OBJC_INLINE void *objc_zero_alloc(unsigned long size) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *objc_zero_alloc(unsigned long size){
	return calloc(1, size);
}
OBJC_INLINE void objc_dealloc(void *memory) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_dealloc(void *memory){
	free(memory);
}

OBJC_INLINE objc_rw_lock objc_rw_lock_create(void) OBJC_ALWAYS_INLINE;
OBJC_INLINE objc_rw_lock objc_rw_lock_create(void){
	pthread_rwlock_t *lock = malloc(sizeof(pthread_rwlock_t));
	pthread_rwlock_init(lock, NULL);
	return lock;
}
OBJC_INLINE void objc_rw_lock_destroy(objc_rw_lock lock) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_rw_lock_destroy(objc_rw_lock lock){
	pthread_rwlock_destroy(lock);
	free(lock);
}
OBJC_INLINE void objc_rw_lock_unlock(objc_rw_lock lock) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_rw_lock_unlock(objc_rw_lock lock){
	pthread_rwlock_unlock(lock);
}
OBJC_INLINE void objc_rw_lock_rlock(objc_rw_lock lock) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_rw_lock_rlock(objc_rw_lock lock){
	pthread_rwlock_rdlock(lock);
}
OBJC_INLINE void objc_rw_lock_wlock(objc_rw_lock lock) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_rw_lock_wlock(objc_rw_lock lock){
	pthread_rwlock_wrlock(lock);
}

#include "array-inline.h"
#include "holder-inline.h"

#endif

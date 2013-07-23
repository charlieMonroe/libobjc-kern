#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <pthread.h>

/* LOGGING */
#define objc_log printf

#define objc_debug_log(...)						\
		if (OBJC_DEBUG_LOG) {					\
			objc_log("DEBUG: ");				\
			objc_log(__VA_ARGS__);				\
		}

/* LOCKING */
typedef struct {
	pthread_rwlock_t	lock;
	const char		*name;
} objc_rw_lock;


static inline void objc_rw_lock_init(objc_rw_lock *lock, const char *name){
	objc_debug_log("Initing lock %s at address %p\n", name, lock);
	lock->name = name;
	++objc_lock_count;
	pthread_rwlock_init(&lock->lock, NULL);
}
static inline int objc_rw_lock_rlock(objc_rw_lock *lock){
	objc_debug_log("Read-locking lock %s at address %p\n",
		       lock->name, lock);
	++objc_lock_locked_count;
	return pthread_rwlock_rdlock(&lock->lock);
}
static inline int objc_rw_lock_wlock(objc_rw_lock *lock){
	objc_debug_log("Write-locking lock %s at address %p\n",
		       lock->name, lock);
	++objc_lock_locked_count;
	return pthread_rwlock_wrlock(&lock->lock);
}
static inline int objc_rw_lock_unlock(objc_rw_lock *lock){
	objc_debug_log("Unlocking lock %s at address %p\n",
		       lock->name, lock);
	return pthread_rwlock_unlock(&lock->lock);
}
static inline void objc_rw_lock_destroy(objc_rw_lock *lock){
	objc_debug_log("Destroying lock %s at address %p\n",
		       lock->name, lock);
	pthread_rwlock_destroy(&lock->lock);
}

/* MEMORY */

/**
 * Two macros that fake the type declarations.
 */
#define	MALLOC_DEFINE(type, shortdesc, longdesc) void *type
#define	MALLOC_DECLARE(type)

static inline void *objc_realloc(void *mem, size_t size, void *type) {
	return realloc(mem, size);
}

static inline void *objc_zero_alloc(size_t size, void *type){
	return calloc(1, size);
}

static inline void *objc_alloc(size_t size, void *type){
	return malloc(size);
}
static inline void *objc_alloc_page(void *type){
	return objc_alloc(PAGE_SIZE, type);
}
static inline void objc_dealloc(void *mem, void *type){
	free(mem);
}

/* THREAD */
static inline void objc_yield(void){
	sleep(0);
}

#define objc_abort(reason...)	{					\
					printf(reason);			\
					abort();			\
				}


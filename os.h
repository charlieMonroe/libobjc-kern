
#ifndef OBJC_OS_H_
#define OBJC_OS_H_

#ifndef _KERNEL
	#include <stdlib.h>
	#include <stdio.h>
	#include <stdint.h>
	#include <string.h>
	#include <stddef.h>
	#include <unistd.h>
	#include <pthread.h>
#else /* _KERNEL */
	#include <sys/param.h>
	#include <sys/systm.h>
	#include <sys/proc.h>
	#include <sys/lock.h>
	#include <sys/rwlock.h>
#endif


#define PAGE_SIZE 4096

#define UNREACHABLE(x) __builtin_unreachable()
#define PUBLIC __attribute__ ((visibility("default")))
#define PRIVATE  __attribute__ ((visibility("hidden")))

#define OBJC_DEBUG_LOG 1

/*
 * Some small macros that are used in a few places,
 * mostly for stitching.
 */
#define REALLY_PREFIX_SUFFIX(x, y) x ## y
#define PREFIX_SUFFIX(x, y) REALLY_PREFIX_SUFFIX(x, y)


#define objc_debug_log(...) \
	if (OBJC_DEBUG_LOG) { printf("DEBUG: "); printf(__VA_ARGS__); }


/* For statistics. */
PRIVATE unsigned int objc_lock_count;
PRIVATE unsigned int objc_lock_locked_count;

#ifndef _KERNEL

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


#define objc_log printf

static inline void objc_dealloc(void *mem){
	free(mem);
}

static inline void objc_yield(void){
	sleep(0);
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





#define objc_assert(condition, description...) \
		if (!(condition)){\
			panic(description);\
		}





#else /* _KERNEL */

/* LOCKING */
typedef struct rwlock objc_rw_lock;

static inline const char *objc_rw_lock_get_name(objc_rw_lock *lock){
	return lock->lock_object.lo_name;
}
static inline void objc_rw_lock_init(objc_rw_lock *lock, const char *name){
	objc_debug_log("Initing lock %s at address %p\n", name, lock);
	rw_init(lock, name);
}
static inline int objc_rw_lock_rlock(objc_rw_lock *lock){
	objc_debug_log("Read-locking lock %s at address %p\n",
		       objc_rw_lock_get_name(lock),
		       lock);
	rw_rlock(lock);
	return 0;
}
static inline int objc_rw_lock_wlock(objc_rw_lock *lock){
	objc_debug_log("Write-locking lock %s at address %p\n",
		       objc_rw_lock_get_name(lock),
		       lock);
	rw_wlock(lock);
	return 0;
}
static inline int objc_rw_lock_unlock(objc_rw_lock *lock){
	objc_debug_log("Unlocking lock %s at address %p\n",
		       objc_rw_lock_get_name(lock),
		       lock);
	rw_unlock(lock);
	return 0;
}
static inline void objc_rw_lock_destroy(objc_rw_lock *lock){
	objc_debug_log("Destroying lock %s at address %p\n",
		       objc_rw_lock_get_name(lock),
		       lock);
	rw_destroy(lock);
}


static inline void objc_yield(void){
	pause("objc_yield", 0);
}

#endif /* !_KERNEL */







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



#endif /* !OBJC_OS_H_ */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "ao-ext.h"
#include "../runtime.h"

static void *_zero_alloc(unsigned long size){
	return calloc(1, size);
}
static void _abort(const char *reason){
	printf("__OBJC_ABORT__ - %s", reason);
	abort();
}
static objc_rw_lock _rw_lock_creator(void){
	pthread_rwlock_t *lock = malloc(sizeof(pthread_rwlock_t));
	pthread_rwlock_init(lock, NULL);
	return lock;
}
static void _rw_lock_destroyer(objc_rw_lock lock){
	pthread_rwlock_destroy(lock);
	free(lock);
}

/**
 * Populates the run-time setup with function pointers.
 */
static void _objc_posix_init(void){
	objc_runtime_set_allocator(malloc);
	objc_runtime_set_deallocator(free);
	objc_runtime_set_zero_allocator(_zero_alloc);
	
	objc_runtime_set_abort(_abort);
	
	objc_runtime_set_log(printf);
	
	objc_runtime_set_rw_lock_creator(_rw_lock_creator);
	objc_runtime_set_rw_lock_destroyer(_rw_lock_destroyer);
	objc_runtime_set_rw_lock_rlock((objc_rw_lock_read_lock_f)pthread_rwlock_rdlock);
	objc_runtime_set_rw_lock_wlock((objc_rw_lock_write_lock_f)pthread_rwlock_wrlock);
	objc_runtime_set_rw_lock_unlock((objc_rw_lock_unlock_f)pthread_rwlock_unlock);
}

/**
 * Registers the posix_init function.
 */
static void _objc_posix_register_initializer(void) __attribute__((constructor));
static void _objc_posix_register_initializer(void){
	objc_runtime_register_initializer(_objc_posix_init);
}

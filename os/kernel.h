#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/rwlock.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/osd.h>

/* LOGGING */
#define objc_log printf

typedef __ptrdiff_t ptrdiff_t;

#define objc_debug_log(...)						\
		if (OBJC_DEBUG_LOG) {					\
			objc_log("DEBUG: ");				\
			objc_log(__VA_ARGS__);				\
		}

/* LOCKING */
typedef struct rwlock objc_rw_lock;

static inline const char *objc_rw_lock_get_name(objc_rw_lock *lock){
	return lock->lock_object.lo_name;
}
static inline void objc_rw_lock_init(objc_rw_lock *lock, const char *name){
	rw_init(lock, name);
}
static inline int objc_rw_lock_rlock(objc_rw_lock *lock){
	rw_rlock(lock);
	return 0;
}
static inline int objc_rw_lock_wlock(objc_rw_lock *lock){
	rw_wlock(lock);
	return 0;
}
static inline int objc_rw_lock_unlock(objc_rw_lock *lock){
	rw_unlock(lock);
	return 0;
}
static inline void objc_rw_lock_destroy(objc_rw_lock *lock){
	rw_destroy(lock);
}


/* MEMORY */
static inline void *objc_alloc(size_t size, struct malloc_type *type){
	return malloc(size, type, 0);
}
static inline void *objc_zero_alloc(size_t size, struct malloc_type *type){
	return calloc(1, type, M_ZERO);
}
static inline void *objc_realloc(void *mem, size_t size,
				 struct malloc_type *type){
	return realloc(mem, size, type, 0);
}
static inline void *objc_alloc_page(struct malloc_type *type){
	return objc_alloc(PAGE_SIZE, type);
}
static inline void objc_dealloc(void *mem, struct malloc_type *type){
	free(mem, type);
}

/* THREAD */
static inline void objc_yield(void){
	pause("objc_yield", 0);
}

#define objc_abort(reason...) panic(reason)

typedef int objc_tls_key;
typedef void(*objc_tls_descructor)(void*);

static inline void objc_register_tls(objc_tls_key *key __unused,
				     objc_tls_descructor destructor){
	*key = osd_thread_register(destructor);
}
static inline void *objc_get_tls_for_key(objc_tls_key key){
	return osd_thread_get(curthread, key);
}
static inline void objc_set_tls_for_key(void *data, objc_tls_key key){
	return osd_thread_set(curthread, key, data);
}


#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/sx.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/osd.h>
#include <ddb/ddb.h>

#include <machine/setjmp.h>

/* LOGGING */
#define objc_log db_printf

typedef __ptrdiff_t ptrdiff_t;

#define objc_debug_log(...)						\
		if (OBJC_DEBUG_LOG) {					\
			objc_log("DEBUG: ");				\
			objc_log(__VA_ARGS__);				\
		}

/* LOCKING */
typedef struct sx objc_rw_lock;

static inline const char *objc_rw_lock_get_name(objc_rw_lock *lock){
	return lock->lock_object.lo_name;
}
static inline void objc_rw_lock_init(objc_rw_lock *lock, const char *name){
	sx_init_flags(lock, name, SX_RECURSE);
}
#define objc_rw_lock_rlock(lock) sx_slock(lock)
/*
static inline int objc_rw_lock_rlock(objc_rw_lock *lock){
	sx_slock(lock);
	return 0;
}
 */
#define objc_rw_lock_wlock(lock) sx_xlock(lock)
/*static inline int objc_rw_lock_wlock(objc_rw_lock *lock){
	sx_xlock(lock);
	return 0;
}*/
static inline int objc_rw_lock_unlock(objc_rw_lock *lock){
	sx_unlock(lock);
	return 0;
}
static inline void objc_rw_lock_destroy(objc_rw_lock *lock){
	sx_destroy(lock);
}

/* THREAD */
static inline void objc_yield(void){
	pause("objc_yield", 0);
}

#define objc_abort(reason...) panic(reason)

typedef int objc_tls_key;
typedef void(*objc_tls_descructor)(void*);

static inline void objc_register_tls(objc_tls_key *key,
				     objc_tls_descructor destructor){
	*key = osd_thread_register(destructor);
}
static inline void objc_deregister_tls(objc_tls_key key) {
	osd_thread_deregister(key);
}
static inline void *objc_get_tls_for_key(objc_tls_key key){
	return osd_thread_get(curthread, key);
}
static inline void objc_set_tls_for_key(void *data, objc_tls_key key){
	osd_thread_set(curthread, key, data);
}

static inline objc_sleep(int secs){
	pause("objc_sleep", secs * 100);
}

/* MEMORY */
static inline void *objc_malloc(size_t size,
				struct malloc_type *type,
				int other_flags)
{
	void *memory = NULL;
	do {
		memory = malloc(size, type, M_WAITOK | other_flags);
		if (memory == NULL) {
			objc_yield();
		}
	} while (memory == NULL);
	return memory;
}
static inline void *objc_alloc(size_t size, struct malloc_type *type){
	return objc_malloc(size, type, 0);
}
static inline void *objc_zero_alloc(size_t size, struct malloc_type *type){
	return objc_malloc(size, type, M_ZERO);
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




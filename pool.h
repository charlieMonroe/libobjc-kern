#include "os.h"

#ifndef POOL_TYPE
#error POOL_TYPE must be defined
#endif
#ifndef POOL_TYPE
#error POOL_NAME must be defined
#endif

#define NAME(x) PREFIX_SUFFIX(POOL_NAME, x)

#define M_POOL_TYPE PREFIX_SUFFIX(M_, POOL_NAME)

MALLOC_DECLARE(M_POOL_TYPE);
static MALLOC_DEFINE(M_POOL_TYPE, "pool", "Objective-C Pool");

/* Malloc one page at a time. */
#define POOL_SIZE ((PAGE_SIZE) / sizeof(POOL_TYPE))
static POOL_TYPE* NAME(_pool);
static int NAME(_pool_next_index) = -1;

#ifdef THREAD_SAFE_POOL
static objc_rw_lock NAME(_lock);
#define LOCK_POOL() LOCK(&POOL_NAME##_lock)
#define UNLOCK_POOL() LOCK(&POOL_NAME##_lock)
#else
#define LOCK_POOL()
#define UNLOCK_POOL()
#endif

static int pool_size = 0;
static int pool_allocs = 0;
static inline POOL_TYPE*NAME(_pool_alloc)(void)
{
	LOCK_POOL();
	pool_allocs++;
	if (0 > NAME(_pool_next_index))
	{
		NAME(_pool) = objc_alloc_page(M_POOL_TYPE);
		NAME(_pool_next_index) = POOL_SIZE - 1;
		pool_size += PAGE_SIZE;
	}
	POOL_TYPE* new = &NAME(_pool)[NAME(_pool_next_index)--];
	UNLOCK_POOL();
	return new;
}
#undef NAME
#undef POOL_SIZE
#undef PAGE_SIZE
#undef POOL_NAME
#undef POOL_TYPE
#undef LOCK_POOL
#undef UNLOCK_POOL
#ifdef THREAD_SAFE_POOL
#undef THREAD_SAFE_POOL
#endif

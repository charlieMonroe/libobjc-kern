
#ifndef POOL_TYPE
#error POOL_TYPE must be defined
#endif
#ifndef POOL_TYPE
#error POOL_NAME must be defined
#endif
#ifndef POOL_MALLOC_TYPE
#error POOL_MALLOC_TYPE must be defined
#endif

#define NAME(x) PREFIX_SUFFIX(POOL_NAME, x)

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

struct NAME(_pool_page) {
	struct NAME(_pool_page) *next;
	void *page;
};

static struct NAME(_pool_page) *NAME(_pages);

static int pool_size = 0;
static int pool_allocs = 0;
static inline POOL_TYPE*NAME(_pool_alloc)(void)
{
	LOCK_POOL();
	pool_allocs++;
	if (0 > NAME(_pool_next_index))
	{
		struct NAME(_pool_page) *page = objc_alloc(sizeof(struct NAME(_pool_page)),
												   POOL_MALLOC_TYPE);
		page->next = NAME(_pages);
		page->page = objc_alloc_page(POOL_MALLOC_TYPE);
		NAME(_pages) = page;
		
		NAME(_pool) = page->page;
		NAME(_pool_next_index) = POOL_SIZE - 1;
		pool_size += PAGE_SIZE;
	}
	POOL_TYPE* new = &NAME(_pool)[NAME(_pool_next_index)--];
	UNLOCK_POOL();
	return new;
}

static void NAME(_pool_free_page)(struct NAME(_pool_page) *page){
	if (page->next != NULL){
		NAME(_pool_free_page)(page->next);
	}
	
	objc_dealloc(page->page, POOL_MALLOC_TYPE);
	objc_dealloc(page, POOL_MALLOC_TYPE);
}

static inline void NAME(_pool_free)(void){
	NAME(_pool_free_page)(NAME(_pages));
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

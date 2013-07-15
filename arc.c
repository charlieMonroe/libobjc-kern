#include "arc.h"
#include "os.h"
#include "message.h"
#include "selector.h"
#include "private.h"
#include "utils.h"

/**
 * Each autorelease pool ~ a page. We need the previous and top
 * pointers, however, so the first two slots are taken.
 */
#define AUTORELEASE_POOL_SIZE ((PAGE_SIZE / sizeof(void*)) - 2)

typedef struct objc_autorelease_pool_struct {
	struct objc_autorelease_pool_struct *previous;
	id *top;
	
	id pool[AUTORELEASE_POOL_SIZE];
} objc_autorelease_pool;

typedef struct {
	objc_autorelease_pool *pool;
	
	id return_retained; // TODO - do we need it when we're using own pools?
	
} objc_arc_thread_data;

typedef struct {
	Class isa;
	int retain_count;
} *Object;

// Forward declarations of inline functions:
static inline objc_autorelease_pool *_objc_create_pool_if_necessary(objc_arc_thread_data *data);

static inline objc_arc_thread_data *_objc_get_arc_thread_data(void){
	// TODO
	return NULL;
}

static int objc_autorelease_object_count = 0;
//static int objc_autorelease_pool_count = 0;

// Initialized in the objc_arc_init();
static SEL objc_retain_selector = 0;
static SEL objc_release_selector = 0;
static SEL objc_dealloc_selector = 0;
static SEL objc_autorelease_selector = 0;

static inline id _objc_retain(id obj){
	// TODO small objects
	// TODO blocks
	if (obj->isa->flags.has_custom_arr){
		// The class has custom ARR methods,
		// send the message
		return objc_msg_send(obj, objc_retain_selector);
	}
	
	/**
	 * The kernel objc run-time assumes
	 * that the objects actually have the retain count variable
	 * directly after the isa pointer.
	 */
	Object object = (Object)obj;
	__sync_add_and_fetch(&(object->retain_count), 1);
	return obj;
}

static inline void _objc_release(id obj){
	// TODO small objects + blocks
	if (obj->isa->flags.has_custom_arr){
		// The class has custom ARR methods,
		// send the message directly
		objc_msg_send(obj, objc_release_selector);
		return;
	}
	
	/**
	 * The kernel objc run-time assumes
	 * that the objects actually have the retain count variable
	 * directly after the isa pointer.
	 */
	Object object = (Object)obj;
	if (__sync_sub_and_fetch(&(object->retain_count), 1) < 0){
		objc_delete_weak_refs(obj);
		objc_msg_send(obj, objc_dealloc_selector);
	}
}

static inline id _objc_autorelease(id obj){
	if (obj->isa->flags.has_custom_arr){
		// Let the object's method handle it
		return objc_msg_send(obj, objc_autorelease_selector);
	}
	
	objc_arc_thread_data *data = _objc_get_arc_thread_data();
	if (data != NULL){
		objc_autorelease_pool *pool = _objc_create_pool_if_necessary(data);
		
		++objc_autorelease_object_count;
		*pool->top = obj;
		++pool->top;
		return obj;
	}
	
	// TODO
	return objc_msg_send(obj, objc_autorelease_selector);
}


/**
 * Empties autorelease pools until it hits the stop pointer.
 */
static void _objc_empty_pool_until(objc_arc_thread_data *data, id *stop){
	objc_autorelease_pool *stop_pool = NULL;
	if (stop != NULL){
		stop_pool = data->pool;
		while (YES) {
			if (stop_pool == NULL){
				// We haven't found the stop point in the pool list
				return;
			}
			
			if ((stop >= stop_pool->pool) && (stop < &stop_pool->pool[AUTORELEASE_POOL_SIZE])){
				// If the stop location is in this pool, stop
				break;
			}
			
			stop_pool = stop_pool->previous;
		}
	}
	
	while (data->pool != stop_pool) {
		/**
		 * We need to operate with (data->pool) and not just
		 * a cached ptr since the release may cause some
		 * additional autoreleases, which could cause
		 * a new pool to be installed.
		 */
		while (data->pool->top > data->pool->pool) {
			--data->pool->top;
			
			_objc_release(*data->pool->top);
			--objc_autorelease_object_count;
		}
		
		// Dispose of the pool itself
		objc_autorelease_pool *pool = data->pool;
		data->pool = pool->previous;
		objc_dealloc(pool);
	}
	
	/**
	 * We've reached a point where data->pool == stop_pool.
	 * Now, if there actually is a pool to be dealt with, we need
	 * to reach the stop point in that pool.
	 */
	if (data->pool != NULL){
		while ((stop == NULL || (data->pool->top > stop)) &&
		       (data->pool->top > data->pool->pool)) {
			--data->pool->top;
			
			_objc_release(*data->pool->top);
			--objc_autorelease_object_count;
		}
	}
}

/**
 * Empties all the pools in the thread data supplied.
 */
static void _objc_cleanup_pools(objc_arc_thread_data *data){
	if (data->return_retained != nil){
		_objc_release(data->return_retained);
		data->return_retained = nil;
	}
	
	if (data->pool != NULL){
		_objc_empty_pool_until(data, NULL);
		objc_assert(data->pool == NULL, "The pool should have been emptied!\n");
	}
	
}

static inline objc_autorelease_pool *_objc_create_pool_if_necessary(objc_arc_thread_data *data){
	objc_autorelease_pool *pool = data->pool;
	if (pool == NULL || pool->top >= &pool->pool[AUTORELEASE_POOL_SIZE]){
		/**
		 * Either there is no pool, or we've run out of space in
		 * the existing one.
		 */
		pool = objc_alloc_page();
		pool->previous = data->pool;
		pool->top = pool->pool;
		data->pool = pool;
	}
	return pool;
}



#pragma mark -
#pragma mark Public functions
#pragma mark Autorelease Pool

void *objc_autorelease_pool_push(void){
	objc_arc_thread_data *data = _objc_get_arc_thread_data();
	
	// TODO data->return_retained?
	
	if (data != NULL){
		objc_autorelease_pool *pool = _objc_create_pool_if_necessary(data);
		return pool->top;
	}
	
	objc_log("An issue occurred when pushing a pool"
		 " - thread data %p, data->pool %p\n",
		 data, data == NULL ? 0 : data->pool);
	
	return NULL;
}

void objc_autorelease_pool_pop(void *pool){
	objc_arc_thread_data *data = _objc_get_arc_thread_data();
	if (data != NULL && data->pool != NULL){
		_objc_empty_pool_until(data, pool);
	}else{
		objc_log("An issue occurred when popping a pool %p"
			 " - thread data %p, data->pool %p\n",
			 pool, data, data == NULL ? 0 : data->pool);
	}
}

#pragma mark -
#pragma mark Ref Count Management

id objc_autorelease(id obj){
	if (obj != nil){
		obj = _objc_autorelease(obj);
	}
	return obj;
}
id objc_retain(id obj){
	if (obj != nil){
		obj = _objc_retain(obj);
	}
	return obj;
}
void objc_release(id obj){
	if (obj != nil){
		_objc_release(obj);
	}
}
id objc_retain_autorelease(id obj){
	return objc_autorelease(objc_retain(obj));
}
id objc_store_strong(id *addr, id value){
	value = objc_retain(value);
	objc_release(*addr);
	*addr = value;
	return value;
}


#pragma mark -
#pragma mark Weak Refs

/**
 * We're using 6 ref ptrs since including the
 * obj and next ptrs, this puts us at 8 pointers
 * in the structure, which is 32 bytes for for a
 * 32-bit computer and 64 bytes for a 64-bit
 * computer.
 */
#define WEAK_REF_SIZE 6

typedef struct objc_weak_ref_struct {
	id obj;
	id *refs[WEAK_REF_SIZE];
	struct objc_weak_ref_struct *next;
} objc_weak_ref;

static objc_weak_ref objc_null_weak_ref;

static int _objc_weak_ref_equal(const id obj, const objc_weak_ref ref){
	return ref.obj == obj;
}

static inline int objc_weak_ref_is_nil(const objc_weak_ref ref){
	return ref.obj == nil;
}

static unsigned int _objc_weak_ref_hash(const objc_weak_ref ref){
	return objc_hash_pointer(ref.obj);
}

#define MAP_TABLE_NAME objc_weak_ref
#define MAP_TABLE_COMPARE_FUNCTION _objc_weak_ref_equal
#define MAP_TABLE_HASH_KEY objc_hash_pointer
#define MAP_TABLE_HASH_VALUE _objc_weak_ref_hash
#define MAP_TABLE_KEY_TYPE id
#define MAP_TABLE_VALUE_TYPE objc_weak_ref
#define MAP_PLACEHOLDER_VALUE objc_null_weak_ref
#define MAP_NULL_EQUALITY_FUNCTION objc_weak_ref_is_nil
#include "hashtable.h"



void objc_arc_init(void){
	objc_autorelease_selector = objc_selector_register("autorelease", "@@:");
	objc_release_selector = objc_selector_register("release", "v@:");
	objc_retain_selector = objc_selector_register("retain", "@@:");
	objc_dealloc_selector = objc_selector_register("dealloc", "v@:");
	
	
	
	// TODO create key for TLS
}

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "kernobjc/arc.h"
#include "kernobjc/message.h"
#include "selector.h"
#include "message.h"
#include "class.h"
#include "associative.h"
#include "init.h"

/*
 * Each autorelease pool ~ a page. We need the previous and top
 * pointers, however, so the first two slots are taken.
 */
#define AUTORELEASE_POOL_SIZE ((PAGE_SIZE / sizeof(void*)) - 2)

/* Default hook for _objc_weak_load() */
static id
__objc_weak_load_default_hook(id object) { return object; }

/*
 * Returns the object if it is not currently in the process of being
 * deallocated.  Returns nil otherwise.
 *
 * This hook must be set for weak references to work with automatic reference 
 * counting.
 */
id (*_objc_weak_load)(id object) = __objc_weak_load_default_hook;

struct objc_autorelease_pool {
	struct objc_autorelease_pool *previous;
	id *top;
	
	id pool[AUTORELEASE_POOL_SIZE];
};

struct objc_arc_thread_data {
	struct objc_autorelease_pool *pool;
};

struct object {
	Class isa;
	int retain_count;
};

static int objc_autorelease_object_count = 0;
static objc_tls_key objc_autorelease_pool_tls_key = 0;

/* Forward declarations of inline functions: */
static inline struct objc_autorelease_pool *
_objc_create_pool_if_necessary(struct objc_arc_thread_data *data);


static inline struct objc_arc_thread_data *
_objc_get_arc_thread_data(void){
	return (struct objc_arc_thread_data*)
			objc_get_tls_for_key(objc_autorelease_pool_tls_key);
}

static inline id
_objc_retain(id obj)
{
	if (objc_object_is_small_object(obj)) {
		return obj;
	}
	
	// TODO blocks
	if (obj->isa->flags.has_custom_arr) {
		/* The class has custom ARR methods, send the message. */
		return objc_send_retain_msg(obj);
	}
	
	/*
	 * The kernel objc run-time assumes
	 * that the objects actually have the retain count variable
	 * directly after the isa pointer.
	 */
	struct object *object = (struct object *)obj;
	__sync_add_and_fetch(&(object->retain_count), 1);
	return obj;
}

static inline void
_objc_release(id obj)
{
	if (objc_object_is_small_object(obj)) {
		return;
	}
	
	// TODO blocks
	if (obj->isa->flags.has_custom_arr) {
		/* The class has custom ARR methods, send the message. */
		objc_send_release_msg(obj);
		return;
	}
	
	/*
	 * The kernel objc run-time assumes
	 * that the objects actually have the retain count variable
	 * directly after the isa pointer.
	 */
	struct object *object = (struct object *)obj;
	if (__sync_sub_and_fetch(&(object->retain_count), 1) < 0) {
		objc_delete_weak_refs(obj);
		objc_send_dealloc_msg(obj);
	}
}

static inline id
_objc_autorelease(id obj)
{
	if (objc_object_is_small_object(obj)) {
		return obj;
	}
	
	if (obj->isa->flags.has_custom_arr) {
		/* Let the object's method handle it */
		return objc_send_autorelease_msg(obj);
	}
	
	struct objc_arc_thread_data *data = _objc_get_arc_thread_data();
	if (data != NULL) {
		struct objc_autorelease_pool *pool;
		pool = _objc_create_pool_if_necessary(data);
		
		++objc_autorelease_object_count;
		*pool->top = obj;
		++pool->top;
		return obj;
	}
	
	return objc_send_autorelease_msg(obj);
}


/*
 * Empties autorelease pools until it hits the stop pointer.
 */
static void
_objc_empty_pool_until(struct objc_arc_thread_data *data, id *stop)
{
	struct objc_autorelease_pool *stop_pool = NULL;
	if (stop != NULL) {
		stop_pool = data->pool;
		while (YES) {
			if (stop_pool == NULL){
				/*
				 * We haven't found the stop point in the pool
				 * list.
				 */
				return;
			}
			
			if ((stop >= stop_pool->pool) &&
			    (stop < &stop_pool->pool[AUTORELEASE_POOL_SIZE])) {
				/* If the stop location is in this pool, stop */
				break;
			}
			
			stop_pool = stop_pool->previous;
		}
	}
	
	while (data->pool != stop_pool) {
		/*
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
		
		/* Dispose of the pool itself */
		struct objc_autorelease_pool *pool = data->pool;
		data->pool = pool->previous;
		objc_dealloc(pool, M_AUTORELEASE_POOL_TYPE);
	}
	
	/*
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

/*
 * Empties all the pools in the thread data supplied.
 */
static void
_objc_cleanup_pools(struct objc_arc_thread_data *data)
{
	if (data->pool != NULL){
		_objc_empty_pool_until(data, NULL);
		objc_assert(data->pool == NULL,
			    "The pool should have been emptied!\n");
	}
}

static inline struct objc_autorelease_pool *
_objc_create_pool_if_necessary(struct objc_arc_thread_data *data)
{
	struct objc_autorelease_pool *pool = data->pool;
	if (pool == NULL || pool->top >= &pool->pool[AUTORELEASE_POOL_SIZE]){
		/*
		 * Either there is no pool, or we've run out of space in
		 * the existing one.
		 */
		pool = objc_alloc_page(M_AUTORELEASE_POOL_TYPE);
		pool->previous = data->pool;
		pool->top = pool->pool;
		data->pool = pool;
	}
	return pool;
}



#pragma mark -
#pragma mark Public functions
#pragma mark Autorelease Pool

void *
objc_autoreleasePoolPush(void)
{
	struct objc_arc_thread_data *data = _objc_get_arc_thread_data();
	if (data != NULL){
		struct objc_autorelease_pool *pool;
		pool = _objc_create_pool_if_necessary(data);
		return pool->top;
	}
	
	objc_log("An issue occurred when pushing a pool"
		 " - thread data %p, data->pool %p\n",
		 data, data == NULL ? 0 : data->pool);
	
	return NULL;
}

void
objc_autoreleasePoolPop(void *pool)
{
	struct objc_arc_thread_data *data = _objc_get_arc_thread_data();
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

id
objc_autorelease(id obj)
{
	if (obj != nil){
		obj = _objc_autorelease(obj);
	}
	return obj;
}

id
objc_retain(id obj)
{
	if (obj != nil){
		obj = _objc_retain(obj);
	}
	return obj;
}

id
objc_copy(id obj)
{
	return objc_send_copy_msg(obj);
}

void
objc_release(id obj)
{
	if (obj != nil){
		_objc_release(obj);
	}
}

id
objc_retainAutorelease(id obj)
{
	return objc_autorelease(objc_retain(obj));
}

id
objc_storeStrong(id *addr, id value)
{
	value = objc_retain(value);
	objc_release(*addr);
	*addr = value;
	return value;
}


#pragma mark -
#pragma mark Weak Refs

/*
 * We are using a special association type to implement weak refs.
 * This association type zeroes the *addr on objc_remove_associated_objects().
 *
 * We need to store the same value for key and value in the associated object.
 * To simply remove the weak ref, store nil into the value for key.
 */
objc_rw_lock objc_weak_refs_lock;

id
objc_storeWeak(id *addr, id obj)
{
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	
	id old = *addr;
	if (obj == nil){
		/* Remove the reference by setting it to nil */
		objc_set_associated_object(old, addr, nil,
					   OBJC_ASSOCIATION_WEAK_REF);
		*addr = nil;
		return nil;
	}
	
	// TODO blocks
	
	Class cl = objc_object_get_class_inline(obj);
	if (cl->flags.has_custom_arr){
		obj = _objc_weak_load(obj);
	}else{
		if (((struct object*)obj)->retain_count < 0){
			obj = nil;
		}
	}
	
	objc_set_associated_object(obj, addr, (id)addr,
				   OBJC_ASSOCIATION_WEAK_REF);
	*addr = obj;
	
	return obj;
}

void
objc_delete_weak_refs(id obj)
{
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	
	objc_remove_associated_weak_refs(obj);
}

id
objc_loadWeakRetained(id *addr)
{
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	
	id obj = *addr;
	if (obj == nil){
		return nil;
	}
	
	if (objc_object_is_small_object(obj)){
		return obj;
	}
	
	// TODO malloc block
	
	if (obj->isa->flags.has_custom_arr){
		obj = _objc_weak_load(obj);
	}else{
		if (((struct object *)obj)->retain_count < 0){
			return nil;
		}
	}
	return objc_retain(obj);
}

id
objc_loadWeak(id *obj)
{
	return objc_autorelease(objc_loadWeakRetained(obj));
}

void
objc_copyWeak(id *dest, id *src)
{
	objc_release(objc_initWeak(dest, objc_loadWeakRetained(src)));
}

void
objc_moveWeak(id *dest, id *src)
{
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	
	id obj = *src;
	
	*dest = *src;
	*src = nil;
	
	if (obj != nil){
		/* Set nil for the old reference and add the new reference */
		objc_set_associated_object(obj, src, nil,
					   OBJC_ASSOCIATION_WEAK_REF);
		objc_set_associated_object(obj, dest, (id)dest,
					   OBJC_ASSOCIATION_WEAK_REF);
	}
}

void
objc_destroyWeak(id *obj)
{
	objc_storeWeak(obj, nil);
}

id objc_initWeak(id *object, id value)
{
	*object = nil;
	return objc_storeWeak(object, value);
}

#pragma mark -
#pragma mark Init Function

void
objc_arc_init(void)
{
	objc_rw_lock_init(&objc_weak_refs_lock, "objc_weak_refs_lock");
	objc_register_tls(&objc_autorelease_pool_tls_key,
			  (objc_tls_descructor)_objc_cleanup_pools);
}

void
objc_arc_destroy(void)
{
	objc_rw_lock_destroy(&objc_weak_refs_lock);
	objc_deregister_tls(objc_autorelease_pool_tls_key);
}


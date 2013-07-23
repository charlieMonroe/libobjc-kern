
#include "arc.h"
#include "message.h"
#include "class.h"
#include "associative.h"

/*
 * Each autorelease pool ~ a page. We need the previous and top
 * pointers, however, so the first two slots are taken.
 */
#define AUTORELEASE_POOL_SIZE ((PAGE_SIZE / sizeof(void*)) - 2)

MALLOC_DECLARE(M_AUTORELEASE_POOL);
static MALLOC_DEFINE(M_AUTORELEASE_POOL, "autorelease pool", "Objective-C "
		     "Autorelease Pool");


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


/* Forward declarations of inline functions: */
static inline struct objc_autorelease_pool *
_objc_create_pool_if_necessary(struct objc_arc_thread_data *data);

static inline struct objc_arc_thread_data *
_objc_get_arc_thread_data(void){
	// TODO
	return NULL;
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
	
	// TODO
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
		
		// Dispose of the pool itself
		struct objc_autorelease_pool *pool = data->pool;
		data->pool = pool->previous;
		objc_dealloc(pool, M_AUTORELEASE_POOL);
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
	// TODO to be used as cleanup for TLS data
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
		pool = objc_alloc_page(M_AUTORELEASE_POOL);
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
objc_autorelease_pool_push(void)
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
objc_autorelease_pool_pop(void *pool)
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
objc_retain_autorelease(id obj)
{
	return objc_autorelease(objc_retain(obj));
}

id
objc_store_strong(id *addr, id value)
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
objc_store_weak(id *addr, id obj)
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
objc_load_weak_retained(id *addr)
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
		// TODO
		// obj = _objc_weak_load(obj);
	}else{
		if (((struct object *)obj)->retain_count < 0){
			return nil;
		}
	}
	return objc_retain(obj);
}

id
objc_load_weak(id *obj)
{
	return objc_autorelease(objc_load_weak_retained(obj));
}

void
objc_copy_weak(id *dest, id *src)
{
	objc_release(objc_init_weak(dest, objc_load_weak_retained(src)));
}

void
objc_move_weak(id *dest, id *src)
{
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	
	id obj = *src;
	
	*dest = *src;
	*src = nil;
	
	if (obj != nil){
		// Set nil for the old reference and add the new reference
		objc_set_associated_object(obj, src, nil,
					   OBJC_ASSOCIATION_WEAK_REF);
		objc_set_associated_object(obj, dest, (id)dest,
					   OBJC_ASSOCIATION_WEAK_REF);
	}
}

void
objc_destroy_weak(id *obj)
{
	objc_store_weak(obj, nil);
}

id objc_init_weak(id *object, id value)
{
	*object = nil;
	return objc_store_weak(object, value);
}

#pragma mark -
#pragma mark Init Function

void
objc_arc_init(void)
{
	objc_rw_lock_init(&objc_weak_refs_lock, "objc_weak_refs_lock");
	
	// TODO create key for TLS for the autorelease pool and add _objc_cleanup_pools as destroy hookup
}

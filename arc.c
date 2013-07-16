#include "arc.h"
#include "os.h"
#include "message.h"
#include "selector.h"
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

static inline id _objc_retain(id obj){
	// TODO small objects
	// TODO blocks
	if (obj->isa->flags.has_custom_arr){
		// The class has custom ARR methods,
		// send the message
		return objc_send_retain_msg(obj);
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
		objc_send_release_msg(obj);
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
		objc_send_dealloc_msg(obj);
	}
}

static inline id _objc_autorelease(id obj){
	if (obj->isa->flags.has_custom_arr){
		// Let the object's method handle it
		return objc_send_autorelease_msg(obj);
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
	return objc_send_autorelease_msg(obj);
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
id objc_copy(id obj){
	return objc_send_copy_msg(obj);
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
#define MAP_TABLE_PLACEHOLDER_VALUE objc_null_weak_ref
#define MAP_TABLE_NULL_EQUALITY_FUNCTION objc_weak_ref_is_nil
#define MAP_TABLE_NO_LOCK 1
#define MAP_TABLE_RETURNS_BY_REFERENCE 1
#include "hashtable.h"

static objc_weak_ref_table *objc_weak_refs;
objc_rw_lock objc_weak_refs_lock;

/**
 * Looks for a chain of objc_weak_ref structures associated
 * with obj, returning the one that contains a references to addr,
 * and writing the index in the ->refs field into index, or -1 if not
 * found.
 */
static inline objc_weak_ref *_objc_weak_ref_for_obj_at_addr_no_lock(id *addr, id obj, int *index){
	objc_weak_ref *old_ref = objc_weak_ref_table_get(objc_weak_refs, obj);
	*index = -1;
	while (NULL != old_ref){
		for (int i=0; i < WEAK_REF_SIZE; ++i){
			if (old_ref->refs[i] == addr){
				*index = i;
				break;
			}
		}
		old_ref = old_ref->next;
	}
	return old_ref;
}

/**
 * Returns the last objc_weak_ref reference in the chain.
 */
static inline objc_weak_ref *_objc_weak_ref_last_in_chain(objc_weak_ref *ref){
	while (ref != NULL) {
		if (ref->next == NULL){
			return ref;
		}
		ref = ref->next;
	}
	return NULL;
}

id objc_store_weak(id *addr, id obj){
	id old = *addr;
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	
	int index = 0;
	objc_weak_ref *ref = _objc_weak_ref_for_obj_at_addr_no_lock(addr, old, &index);
	if (ref != NULL){
		ref->refs[index] = 0;
	}
	
	if (obj == nil){
		*addr = nil;
		return nil;
	}
	
	// TODO block classes
	
	ref = _objc_weak_ref_for_obj_at_addr_no_lock(NULL, obj, &index);
	if (ref != NULL && index != -1){
		// Found a place
		ref->refs[index] = addr;
		*addr = obj;
		return obj;
	}
	
	ref = objc_weak_ref_table_get(objc_weak_refs, obj);
	if (ref == NULL){
		// First weak ref for obj
		objc_weak_ref new_ref = {0};
		new_ref.obj = obj;
		new_ref.refs[0] = addr;
		objc_weak_ref_insert(objc_weak_refs, new_ref);
	}else{
		ref = _objc_weak_ref_last_in_chain(ref);
		ref->next = objc_zero_alloc(sizeof(objc_weak_ref));
		ref->next->refs[0] = addr;
	}
	
	*addr = obj;
	return obj;
}

static void _objc_zero_refs(objc_weak_ref *ref, BOOL free){
	if (ref->next != NULL){
		_objc_zero_refs(ref->next, YES);
	}
	
	for (int i = 0; i < WEAK_REF_SIZE; ++i){
		if (ref->refs[i] != NULL){
			*ref->refs[i] = 0;
		}
	}
	
	if (free){
		objc_dealloc(ref);
	}else{
		objc_memory_zero(ref, sizeof(objc_weak_ref));
	}
	
}
void objc_delete_weak_refs(id obj){
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	objc_weak_ref *ref = objc_weak_ref_table_get(objc_weak_refs, obj);
	if (ref != NULL){
		_objc_zero_refs(ref, NO);
	}
}
id objc_load_weak_retained(id *addr){
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	
	id obj = *addr;
	if (obj == nil){
		return nil;
	}
	
	// TODO malloc block
	
	if (obj->isa->flags.has_custom_arr){
		// TODO
		// obj = _objc_weak_load(obj);
	}else{
		if (((Object)obj)->retain_count < 0){
			return nil;
		}
	}
	return objc_retain(obj);
}
id objc_load_weak(id *obj){
	return objc_autorelease(objc_load_weak_retained(obj));
}
void objc_copy_weak(id *dest, id *src){
	objc_release(objc_init_weak(dest, objc_load_weak_retained(src)));
}
void objc_move_weak(id *dest, id *src){
	OBJC_LOCK_FOR_SCOPE(&objc_weak_refs_lock);
	*dest = *src;
	*src = nil;
	
	// Just updating the reference
	
	int index = 0;
	objc_weak_ref *ref = _objc_weak_ref_for_obj_at_addr_no_lock(src, *dest, &index);
	
	if (ref != NULL){
		ref->refs[index] = dest;
	}
	
}
void objc_destroy_weak(id *obj){
	objc_store_weak(obj, nil);
}
id objc_init_weak(id *object, id value){
	*object = nil;
	return objc_store_weak(object, value);
}

#pragma mark -
#pragma mark Init Function

void objc_arc_init(void){
	objc_rw_lock_init(&objc_weak_refs_lock);
	objc_weak_refs = objc_weak_ref_table_create(128);
	
	// TODO create key for TLS
}

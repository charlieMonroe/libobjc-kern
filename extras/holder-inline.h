
#ifndef _INLINE_FUNCTIONS_SAMPLE_INLINE_HOLDER_H_
#define _INLINE_FUNCTIONS_SAMPLE_INLINE_HOLDER_H_

#include "../os.h"
#include "../utils.h"
#include "../private.h"

typedef enum {
	holder_type_selector,
	holder_type_class,
	holder_type_cache
} holder_type;

typedef struct _bucket_struct {
	struct _bucket_struct *next;
	void *obj;
} _bucket;

/**
 * To make things easier (as this is just a sample structure
 * for the run-time), as well as include less dependencies
 * on the run-time itself, and most importantly, because
 * it's enough, there is a fixed number of buckets in the
 * structure.
 */
#define HOLDER_BUCKET_COUNT 64

#define OFFSETOF(type, field) ((unsigned int)&(((type *)0)->field))

/**
 * Structure of the actual holder.
 *
 * lock - RW lock, used only for inserting items.
 * type - one of the types above. Used to determine which key to use.
 * buckets - lazily allocated buckets (i.e. NULL at the beginning). This
 *		  can save memory for classes that aren't used by the program
 *		  so that their cache doesn't get allocated unless needed.
 * key_offset_in_object - offset of the key within the obj structure.
 * readers - how many readers are reading from the structure.
 * dealloc_mark - if the structure is marked to be deallocated. If the mark
 *				is YES the structure gets deallocated once readers is
 *				decreased to 0.
 * pointer_equality - whether to simply compare pointers to determine
 *				equality. Implies pointer hash, instead of key hash
 *				as well.
 */
typedef struct _holder_str {
	objc_rw_lock lock;
	unsigned int key_offset_in_object;
	unsigned int readers;
	BOOL dealloc_mark;
	BOOL pointer_equality;
	_bucket **buckets;
} *_holder;


OBJC_INLINE void _holder_initialize_buckets(_holder holder) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _holder_initialize_buckets(_holder holder){
	holder->buckets = objc_zero_alloc(HOLDER_BUCKET_COUNT * sizeof(_bucket*));
}

OBJC_INLINE unsigned int _bucket_index_for_key(_holder holder, const void *key) OBJC_ALWAYS_INLINE;
OBJC_INLINE unsigned int _bucket_index_for_key(_holder holder, const void *key){
	unsigned long hash = holder->pointer_equality ? (unsigned long)key : objc_hash_string((const char*)key);
	return hash & (HOLDER_BUCKET_COUNT - 1);
}

OBJC_INLINE void *_holder_object_in_bucket(_holder holder, _bucket *bucket, void *obj) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *_holder_object_in_bucket(_holder holder, _bucket *bucket, void *obj){
	while (bucket != NULL) {
		if (bucket->obj == obj){
			/* Already there! */
			return bucket->obj;
		}
		
		bucket = bucket->next;
	}
	return NULL;
}

OBJC_INLINE BOOL _holder_contains_object_in_bucket(_holder holder, _bucket *bucket, void *obj) OBJC_ALWAYS_INLINE;
OBJC_INLINE BOOL _holder_contains_object_in_bucket(_holder holder, _bucket *bucket, void *obj){
	return (BOOL)(_holder_object_in_bucket(holder, bucket, obj) != NULL);
}

OBJC_INLINE void _inline_holder_deallocate_bucket(_bucket *bucket) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _inline_holder_deallocate_bucket(_bucket *bucket){
	_bucket *next_bucket = bucket;
	while (next_bucket != NULL){
		_bucket *current_bucket = next_bucket;
		next_bucket = current_bucket->next;
		
		objc_dealloc(current_bucket);
	}
}

OBJC_INLINE void _holder_deallocate(_holder holder) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _holder_deallocate(_holder holder){
	unsigned int index = 0;
	
	/* Make sure no one is writing. */
	objc_rw_lock_wlock(holder->lock);
	
	for (index = 0; index < HOLDER_BUCKET_COUNT; ++index){
		if (holder->buckets[index] != NULL){
			_inline_holder_deallocate_bucket(holder->buckets[index]);
		}
	}
	
	objc_dealloc(holder->buckets);
	objc_rw_lock_unlock(holder->lock);
	objc_rw_lock_destroy(holder->lock);
}

OBJC_INLINE void *_holder_object_in_bucket_for_key(_holder holder, _bucket *bucket, const void *key) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *_holder_object_in_bucket_for_key(_holder holder, _bucket *bucket, const void *key){
	while (bucket != NULL) {
		void *obj_key = *(void**)(((char*)bucket->obj) + holder->key_offset_in_object);
		BOOL equals = holder->pointer_equality ? (obj_key == key) : objc_strings_equal(key, obj_key);
		if (equals){
			/* Already there! */
			return bucket->obj;
		}
		
		bucket = bucket->next;
	}
	return NULL;
}

OBJC_INLINE void holder_insert_object_internal(_holder holder, void *obj) OBJC_ALWAYS_INLINE;
OBJC_INLINE void holder_insert_object_internal(_holder holder, void *obj){
	unsigned int bucket_index;
	_bucket *bucket;
	const void *key;
	
	if (holder->buckets == NULL){
		_holder_initialize_buckets(holder);
	}
	
	key = *(void**)((char*)obj + holder->key_offset_in_object);
	bucket_index = _bucket_index_for_key(holder, key);
	if (_holder_contains_object_in_bucket(holder, holder->buckets[bucket_index], obj)){
		/* Already contains */
		return;
	}
	
	/*
	 * Prepare the bucket before locking in order to
	 * keep the structure locked as little as possible
	 */
	bucket = (_bucket *)objc_alloc(sizeof(_bucket));
	bucket->obj = obj;
	
	/* Lock the structure for insert */
	objc_rw_lock_wlock(holder->lock);
	
	/* Someone might have inserted it between searching and locking */
	if (_holder_contains_object_in_bucket(holder, holder->buckets[bucket_index], obj)){
		objc_rw_lock_unlock(holder->lock);
		objc_dealloc(bucket);
		return;
	}
	
	bucket->next = holder->buckets[bucket_index];
	holder->buckets[bucket_index] = bucket;
	
	objc_rw_lock_unlock(holder->lock);
}

OBJC_INLINE void *holder_fetch_object(_holder holder, const void *key) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *holder_fetch_object(_holder holder, const void *key){
	_bucket *bucket;
	void *result;
	unsigned int bucket_index;
	
	if (holder->buckets == NULL){
		/* No buckets */
		return NULL;
	}
	
	++holder->readers;
	
	/* Check the dealloc mark one more time. */
	if (holder->dealloc_mark){
		_holder_deallocate(holder);
		return NULL;
	}
	
	bucket_index = _bucket_index_for_key(holder, key);
	bucket = holder->buckets[bucket_index];
	result = _holder_object_in_bucket_for_key(holder, bucket, key);
	--holder->readers;
	
	if (holder->dealloc_mark && holder->readers == 0){
		_holder_deallocate(holder);
	}
	
	
	return result;
}

OBJC_INLINE void holder_mark_to_deallocate(_holder holder) OBJC_ALWAYS_INLINE;
OBJC_INLINE void holder_mark_to_deallocate(_holder holder){
	holder->dealloc_mark = YES;
	if (holder->readers == 0){
		_holder_deallocate(holder);
	}
}

OBJC_INLINE _holder holder_create_internal(holder_type type) OBJC_ALWAYS_INLINE;
OBJC_INLINE _holder holder_create_internal(holder_type type){
	_holder holder = (_holder)(objc_alloc(sizeof(struct _holder_str)));
	holder->buckets = NULL;
	holder->lock = objc_rw_lock_create();
	holder->readers = 0;
	holder->dealloc_mark = NO;
	holder->pointer_equality = NO;
	switch (type) {
		case holder_type_class:
			holder->key_offset_in_object = OFFSETOF(struct objc_class, name);
			break;
		case holder_type_selector:
			holder->key_offset_in_object = OFFSETOF(struct objc_selector, name);
			break;
		case holder_type_cache:
			holder->key_offset_in_object = OFFSETOF(struct objc_method, selector);
			holder->pointer_equality = YES;
			break;
		default:
			objc_abort("Unknown holder type!");
			break;
	}
	
	return holder;
}


OBJC_INLINE objc_selector_holder objc_selector_holder_create(void) OBJC_ALWAYS_INLINE;
OBJC_INLINE objc_selector_holder objc_selector_holder_create(void){
	return (objc_selector_holder)holder_create_internal(holder_type_selector);
}
OBJC_INLINE void objc_selector_holder_insert(objc_selector_holder holder, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_selector_holder_insert(objc_selector_holder holder, SEL selector){
	holder_insert_object_internal((_holder)holder, selector);
}
OBJC_INLINE SEL objc_selector_holder_lookup(objc_selector_holder holder, const char *name) OBJC_ALWAYS_INLINE;
OBJC_INLINE SEL objc_selector_holder_lookup(objc_selector_holder holder, const char *name){
	return (SEL)holder_fetch_object((_holder)holder, name);
}

OBJC_INLINE objc_class_holder objc_class_holder_create(void) OBJC_ALWAYS_INLINE;
OBJC_INLINE objc_class_holder objc_class_holder_create(void){
	return (objc_class_holder)holder_create_internal(holder_type_class);
}
OBJC_INLINE void objc_class_holder_insert(objc_class_holder holder, Class cl) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_class_holder_insert(objc_class_holder holder, Class cl){
	holder_insert_object_internal((_holder)holder, cl);
}
OBJC_INLINE Class objc_class_holder_lookup(objc_class_holder holder, const char *name) OBJC_ALWAYS_INLINE;
OBJC_INLINE Class objc_class_holder_lookup(objc_class_holder holder, const char *name){
	return (Class)holder_fetch_object((_holder)holder, name);
}

OBJC_INLINE objc_cache objc_cache_create(void) OBJC_ALWAYS_INLINE;
OBJC_INLINE objc_cache objc_cache_create(void){
	return (objc_cache)holder_create_internal(holder_type_cache);
}
OBJC_INLINE Method objc_cache_fetch(objc_cache cache, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method objc_cache_fetch(objc_cache cache, SEL selector){
	return holder_fetch_object((_holder)cache, selector);
}
OBJC_INLINE void objc_cache_destroy(objc_cache cache) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_cache_destroy(objc_cache cache){
	holder_mark_to_deallocate((_holder)cache);
}
OBJC_INLINE void objc_cache_insert(objc_cache cache, Method method) OBJC_ALWAYS_INLINE;
OBJC_INLINE void objc_cache_insert(objc_cache cache, Method method){
	holder_insert_object_internal((_holder)cache, method);
}

#endif

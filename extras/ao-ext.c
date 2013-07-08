/**
 * Associated objects class extension.
 */

#include "ao-ext.h"
#include "../classext.h"

objc_class_extension ao_extension;

/** We don't expect many associated objects... */
#define AO_HASH_MAP_BUCKET_COUNT 16

typedef struct _ao_bucket {
	struct _ao_bucket *next;
	void *key;
	id object;
} ao_bucket;

typedef struct {
	ao_bucket **buckets;
} ao_extension_object_part;

static void _deallocate_ao_bucket(ao_bucket *bucket){
	if (bucket->next == NULL){
		return;
	}
	
	_deallocate_ao_bucket(bucket->next);
	objc_dealloc(bucket);
}

static void _associated_object_deallocate(id obj, void *ptr){
	ao_extension_object_part *ext = (ao_extension_object_part*)ptr;
	ao_bucket **hash_map = ext->buckets;
	unsigned int i;
	
	if (hash_map == NULL){
		return;
	}
	
	for (i = 0; i < AO_HASH_MAP_BUCKET_COUNT; ++i){
		_deallocate_ao_bucket(hash_map[i]);
	}
	objc_dealloc(hash_map);
}

void objc_associated_object_register_extension(void){
	ao_extension.class_initializer = NULL;
	ao_extension.class_lookup_function = NULL;
	ao_extension.extra_class_space = 0;
	ao_extension.extra_object_space = sizeof(ao_extension_object_part);
	ao_extension.instance_lookup_function = NULL;
	ao_extension.object_destructor = _associated_object_deallocate;
	ao_extension.object_initializer = NULL; /* Lazy allocation */
	
	objc_class_add_extension(&ao_extension);
}

static void objc_associated_object_register_initializer(void){
	objc_runtime_register_initializer(objc_associated_object_register_extension);
}


/**
 * Returns an object associated with obj under key.
 */
id objc_object_get_associated_object(id obj, void *key){
	ao_extension_object_part *ext;
	ao_bucket **hash_map;
	
	if (obj == NULL || key == NULL){
		return NULL;
	}
	
	ext = (ao_extension_object_part*)objc_object_extensions_beginning_for_extension(obj, &ao_extension);
	hash_map = ext->buckets;
	if (hash_map == NULL){
		return nil;
	}else{
		unsigned int bucket_index = ((unsigned int)key & (AO_HASH_MAP_BUCKET_COUNT - 1));
		ao_bucket *bucket = hash_map[bucket_index];
		
		while (bucket != NULL) {
			if (bucket->key == key){
				return bucket->object;
			}
			bucket = bucket->next;
		}
		
		return nil;
	}
}

/**
 * Allocates a bucket and populates it.
 */
OBJC_INLINE ao_bucket *_create_bucket_with_key_and_value(void *key, id value) OBJC_ALWAYS_INLINE;
OBJC_INLINE ao_bucket *_create_bucket_with_key_and_value(void *key, id value){
	ao_bucket *new_bucket = objc_alloc(sizeof(struct _ao_bucket));
	new_bucket->next = NULL;
	new_bucket->key = key;
	new_bucket->object = value;
	return new_bucket;
}


/**
 * Sets an object value associated with obj under key.
 */
void objc_object_set_associated_object(id obj, void *key, id value){
	ao_extension_object_part *ext;
	ao_bucket **hash_map;
	unsigned int bucket_index;
	ao_bucket *bucket;
	
	if (obj == nil || key == NULL){
		return;
	}
	
	ext = (ao_extension_object_part*)objc_object_extensions_beginning_for_extension(obj, &ao_extension);
	hash_map = ext->buckets;
	
	if (hash_map == NULL){
		hash_map = objc_zero_alloc(sizeof(ao_bucket *) * AO_HASH_MAP_BUCKET_COUNT);
		ext->buckets = hash_map;
	}
	
	bucket_index = ((unsigned int)key & (AO_HASH_MAP_BUCKET_COUNT - 1));
	bucket = hash_map[bucket_index];
	
	if (bucket == NULL){
		hash_map[bucket_index] = _create_bucket_with_key_and_value(key, value);
		return;
	}
	
	while (YES) {
		if (bucket->key == key){
			bucket->object = value;
			return;
		}
		
		if (bucket->next == NULL){
			/* Need to create a new bucket. */
			ao_bucket *new_bucket = _create_bucket_with_key_and_value(key, value);
			bucket->next = new_bucket;
			return;
		}
		
		bucket = bucket->next;
	}
	
}




#include "selector.h"
#include "utils.h" /* For strcpy */

/**
 * Since the SEL is a 16-bit integer, all we need to
 * have are 2-level sparse arrays that get slowly filled,
 * the index in the sparse array is the actual SEL which
 * means quick SEL -> selector name conversion.
 *
 * To efficiently register selectors, however, it is necessary
 * to keep an additional hash table that hashes name -> SEL.
 */

/**
 * The initial capacity for the hash table.
 */
#define OBJC_TABLE_INITIAL_CAPACITY 1024


static inline BOOL _objc_selector_structs_are_equal(Selector sel1, Selector sel2){
	if (sel1 == NULL && sel2 == NULL){
		// Who really can say?
		return YES;
	}
	if (sel1 == NULL || sel2 == NULL){
		// Just one of them
		return NO;
	}
	
	BOOL result = sel1->selUID == sel2->selUID ? YES : NO;
	return result;
}

static inline uint32_t _objc_selector_hash(Selector sel){
	objc_assert(sel != NULL, "Can't hash NULL selector!");
	return objc_hash_string(sel->name);
}

#define MAP_TABLE_NAME objc_selector
#define MAP_TABLE_COMPARE_FUNCTION _objc_selector_structs_are_equal
#define MAP_TABLE_HASH_KEY _objc_selector_hash
#define MAP_TABLE_HASH_VALUE _objc_selector_hash
#define MAP_TABLE_VALUE_TYPE Selector

#include "hashtable.h"

static objc_rw_lock string_allocator_lock;
static char *string_allocator = NULL;
static size_t string_allocator_bytes_remaining = 0;

static char *_objc_selector_allocate_string(size_t size){
	objc_rw_lock_wlock(&string_allocator_lock);
	
	if (string_allocator_bytes_remaining < size){
		// This also covers the case when string_allocator is NULL
		string_allocator = objc_alloc_page();
		string_allocator_bytes_remaining = PAGE_SIZE;
		
		if (string_allocator == NULL){
			return NULL;
		}
	}
	
	char *result = string_allocator;
	string_allocator += size;
	string_allocator_bytes_remaining -= size;
	
	objc_rw_lock_unlock(&string_allocator_lock);
	
	return result;
}

static char *_objc_selector_copy_name_and_types(const char *name, const char *types){
	size_t name_size = objc_strlen(name);
	size_t types_size = objc_strlen(types);
	size_t size = name_size + types_size + 2; // +2 for two \0
	
	char *result = _objc_selector_allocate_string(size);
	
	memcpy(result, name, name_size + 1);
	memcpy(result + name_size + 1, types, types_size + 1);
	
	return result;
}



// TODO initialize
struct objc_selector_table_struct *objc_selector_hashtable;
static void *selector_sparse;

static BOOL objc_selector_register_direct(Selector selector) {
	objc_assert(selector != NULL, "Registering NULL selector!");
	
	// TODO remove
	static int counter = 1;
	selector->selUID = counter;
	++counter;
	
	if (objc_selector_insert(objc_selector_hashtable, selector) == 0){
		printf("Failed to insert selector %s!\n", selector->name);
		return NO;
	}
	
	//
	// TODO
	// Insert into sparse array
	
	return YES;
}


/* Public functions, documented in the header file. */
SEL objc_selector_register(const char *name, const char *types){
	objc_assert(name != NULL, "Cannot register a selector with NULL name!");
	objc_assert(types != NULL, "Cannot register a selector with NULL types!");
	objc_assert(objc_strlen(types) > 2, "Not enough types for registering selector.");
	
	struct objc_selector fake_sel = {
		// We don't need to do anything about
		// types since the hash function stops at \0
		// anyway
		.name = name,
		.selUID = 0
	};
	
	Selector selector = objc_selector_table_get(objc_selector_hashtable, &fake_sel);
	if (selector == NULL){
		selector = objc_alloc(sizeof(struct objc_selector));
		selector->name = _objc_selector_copy_name_and_types(name, types);
		selector->selUID = 0; // Will be populated when registered
		
		if (selector->name == NULL){
			// Probably run out of memory
			return 0;
		}
		
		if (!objc_selector_register_direct(selector)){
			return 0;
		}
	}else{
		const char *selector_types = selector->name + objc_strlen(selector->name) + 1;
		objc_assert(objc_strings_equal(types, selector_types), "Trying to register a"
			    " selector with the same name but different types!\n");
	}
	
	return selector->selUID;
}

const char *objc_selector_get_name(SEL selector){
	if (selector == 0){
		return "((null))";
	}
	
	/**
	Selector selector = objc_selector_table_get(objc_selector_hashtable, selector);
	objc_assert(selector != NULL, "Trying to get name from an unregistered selector.");
	return selector->name;
	 */
	// TODO get it from the sparse array
	return "";
}

const char *objc_selector_get_types(SEL selector){
	if (selector == 0){
		return "";
	}
	/*
	Selector selector = objc_selector_hashtable_lookup(selector_cache, name);
	objc_assert(selector != NULL, "Trying to get types from an unregistered selector.");
	return selector->types;
	 */
	// TODO get it from the sparse array
	return "";
}

void objc_selector_init(void){
	// Assert that the runtime initialization lock is locked.
	
	objc_selector_hashtable = objc_selector_table_create(OBJC_TABLE_INITIAL_CAPACITY);
	objc_rw_lock_init(&string_allocator_lock);
	// TODO init sparse
}


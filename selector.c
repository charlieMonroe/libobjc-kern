
#include "selector.h"
#include "utils.h" /* For strcpy */
#include "sarray2.h"

/**
 * Since the SEL is a 16-bit integer, all we need to
 * have are 2-level sparse arrays that get slowly filled,
 * the index in the sparse array is the actual SEL which
 * means quick SEL -> selector name conversion.
 *
 * To efficiently register selectors, however, it is necessary
 * to keep an additional hash table that hashes name -> SEL.
 */

#pragma mark -
#pragma mark STATIC_VARIABLES_AND_MACROS

/**
 * The initial capacity for the hash table.
 */
#define OBJC_TABLE_INITIAL_CAPACITY 1024

// Forward declarations needed for the hash table
static inline BOOL _objc_selector_structs_are_equal(Selector sel1, Selector sel2);
static inline uint32_t _objc_selector_hash(Selector sel);

#define MAP_TABLE_NAME objc_selector
#define MAP_TABLE_COMPARE_FUNCTION _objc_selector_structs_are_equal
#define MAP_TABLE_HASH_KEY _objc_selector_hash
#define MAP_TABLE_HASH_VALUE _objc_selector_hash
#define MAP_TABLE_VALUE_TYPE Selector

#include "hashtable.h"

/** String allocator stuff.*/
static char *string_allocator = NULL;
static size_t string_allocator_bytes_remaining = 0;

static objc_rw_lock objc_selector_lock;

struct objc_selector_table_struct *objc_selector_hashtable;
static SparseArray *objc_selector_sparse;


#pragma mark -
#pragma mark PRIVATE_FUNCTIONS

/**
 * Returns YES if both selector structures are equal
 * based on string comparison of names.
 */
static inline BOOL _objc_selector_structs_are_equal(Selector sel1, Selector sel2){
	if (sel1 == NULL && sel2 == NULL){
		// Who really can say?
		return YES;
	}
	if (sel1 == NULL || sel2 == NULL){
		// Just one of them
		return NO;
	}
		
	/**
	 * WARNING: Need to compare strings, not just selUID
	 * since the hashtable uses this even for lookup, where
	 * the selUID is unknown (and hence 0).
	 */
	return objc_strings_equal(sel1->name, sel2->name);
}

/**
 * Hashes the selector using its name field.
 */
static inline uint32_t _objc_selector_hash(Selector sel){
	objc_assert(sel != NULL, "Can't hash NULL selector!");
	return objc_hash_string(sel->name);
}

static inline const char *_objc_selector_get_types(Selector sel){
	return sel->name + objc_strlen(sel->name) + 1;
}

/**
 * Uses direct page allocation for string allocation.
 *
 * Requires objc_selector_lock to be locked.
 */
static char *_objc_selector_allocate_string(size_t size){
	/**
	 * No locking necessary, since this gets called
	 * only from places, where the objc_selector_lock
	 * is already locked.
	 */
	
	if (string_allocator_bytes_remaining < size){
		/**
		 * This means that we need more bytes than are left
		 * on that page.
		 *
		 * We of course could keep pointer to the old page,
		 * and try to fill the remaining few bytes, but it will 
		 * be most often just a really few bytes since the selector
		 * and type strings are usually quite small.
		 *
		 * This also covers the string_allocator == NULL case.
		 */
		
		string_allocator = objc_alloc_page();
		string_allocator_bytes_remaining = PAGE_SIZE;
		
		if (string_allocator == NULL){
			return NULL;
		}
	}
	
	char *result = string_allocator;
	string_allocator += size;
	string_allocator_bytes_remaining -= size;
	
	return result;
}

/**
 * Creates a new string including both name and types
 * and copies both strings over.
 *
 * Requires objc_selector_lock to be locked.
 */
static char *_objc_selector_copy_name_and_types(const char *name, const char *types){
	size_t name_size = objc_strlen(name);
	size_t types_size = objc_strlen(types);
	size_t size = name_size + types_size + 2; // +2 for two \0
	
	char *result = _objc_selector_allocate_string(size);
	
	memcpy(result, name, name_size + 1);
	memcpy(result + name_size + 1, types, types_size + 1);
	
	return result;
}

/**
 * Inserts selector into the hashtable and sparse array.
 *
 * Requires objc_selector_lock to be locked.
 */
static BOOL objc_selector_register_direct(Selector selector) {
	objc_assert(selector != NULL, "Registering NULL selector!");
	
	static int objc_selector_counter = 1;
	int original_value = __sync_fetch_and_add(&objc_selector_counter, 1);
	selector->selUID = original_value;
	
	if (objc_selector_insert(objc_selector_hashtable, selector) == 0){
		printf("Failed to insert selector %s!\n", selector->name);
		return NO;
	}
	
	SparseArrayInsert(objc_selector_sparse, selector->selUID, selector);
	
	return YES;
}


/******* PUBLIC FUNCTIONS *******/
#pragma mark -
#pragma mark PUBLIC_FUNCTIONS

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
		/**
		 * Lock for rw access, try to look up again if some
		 * other thread hasn't inserted the selector and
		 * if not, allocate the structure and insert it.
		 */
		
		objc_rw_lock_wlock(&objc_selector_lock);
		
		selector = objc_selector_table_get(objc_selector_hashtable, &fake_sel);
		if (selector == NULL){
			/**
			 * Still NULL -> no other thread inserted it
			 * in the meanwhile.
			 */
			
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
		}
		
		objc_rw_lock_unlock(&objc_selector_lock);
	}else{
		const char *selector_types = _objc_selector_get_types(selector);
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
	 * No locking necessary since we don't remove selectors
	 * from the run-time, which means the sparse array isn't
	 * going anywhere.
	 */
	
	Selector sel_struct = (Selector)SparseArrayLookup(objc_selector_sparse, selector);
	objc_assert(sel_struct != NULL, "Trying to get name from an unregistered selector.");
	return sel_struct->name;
}

const char *objc_selector_get_types(SEL selector){
	if (selector == 0){
		return "";
	}
	
	/**
	 * No locking necessary since we don't remove selectors
	 * from the run-time, which means the sparse array isn't
	 * going anywhere.
	 */
	
	Selector sel_struct = (Selector)SparseArrayLookup(objc_selector_sparse, selector);
	objc_assert(sel_struct != NULL, "Trying to get types from an unregistered selector.");
	
	// The types are in the same string as name, separated by \0
	return _objc_selector_get_types(sel_struct);
}


/******* INIT FUNCTION *******/
#pragma mark -
#pragma mark INIT_FUNCTION

void objc_selector_init(void){
	// Assert that the runtime initialization lock is locked.
	
	/**
	  * Init the hash table that is used for getting selector
	  * name for SEL.
	  */
	objc_selector_hashtable = objc_selector_table_create(OBJC_TABLE_INITIAL_CAPACITY);
	
	/**
	 * Init RW lock for locking the string allocator. The
	 * allocator itself is lazily allocated.
	 */
	objc_rw_lock_init(&objc_selector_lock);
	
	/**
	 * Sparse array holding the SEL -> Selector mapping.
	 */
	objc_selector_sparse = SparseArrayNew();
}


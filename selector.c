
#include "selector.h"
#include "utils.h" /* For strcpy */
#include "sarray2.h"

/*
 * Since the SEL is a 16-bit integer, all we need to
 * have are 2-level sparse arrays that gets slowly filled,
 * the index in the sparse array is the actual SEL which
 * means quick SEL -> selector name conversion.
 *
 * To efficiently register selectors, however, it is necessary
 * to keep an additional hash table that hashes name -> SEL.
 */

#pragma mark -
#pragma mark STATIC_VARIABLES_AND_MACROS

/*
 * The initial capacity for the hash table.
 */
#define OBJC_SELECTOR_TABLE_INITIAL_CAPACITY 1024

/* Initialized in the objc_selector_init(); */
PRIVATE SEL objc_retain_selector = null_selector;
PRIVATE SEL objc_release_selector = null_selector;
PRIVATE SEL objc_dealloc_selector = null_selector;
PRIVATE SEL objc_autorelease_selector = null_selector;
PRIVATE SEL objc_copy_selector = null_selector;
PRIVATE SEL objc_cxx_construct_selector = null_selector;
PRIVATE SEL objc_cxx_destruct_selector = null_selector;
PRIVATE SEL objc_load_selector = null_selector;
PRIVATE SEL objc_initialize_selector = null_selector;
PRIVATE SEL objc_is_arc_compatible_selector = null_selector;


/* Forward declarations needed for the hash table */
static inline BOOL _objc_selector_struct_name_is_equal_to(void *key,
						struct objc_selector *sel);
static inline uint32_t _objc_selector_hash(struct objc_selector *sel);

#define MAP_TABLE_NAME objc_selector
#define MAP_TABLE_COMPARE_FUNCTION _objc_selector_struct_name_is_equal_to
#define MAP_TABLE_HASH_KEY objc_hash_string
#define MAP_TABLE_HASH_VALUE _objc_selector_hash
#define MAP_TABLE_VALUE_TYPE struct objc_selector *
#define MAP_TABLE_NO_LOCK 1
#define MAP_MALLOC_TYPE M_SELECTOR_MAP_TYPE

#include "hashtable.h"


/* String allocator stuff.*/
static char *string_allocator = NULL;
static size_t string_allocator_bytes_remaining = 0;

static objc_rw_lock objc_selector_lock;

struct objc_selector_table_struct *objc_selector_hashtable;
static SparseArray *objc_selector_sparse;


#pragma mark -
#pragma mark PRIVATE_FUNCTIONS

/*
 * Returns YES if both selector structures are equal
 * based on string comparison of names.
 */
static inline BOOL
_objc_selector_struct_name_is_equal_to(void *key, struct objc_selector *sel)
{
	return objc_strings_equal(sel->name, (const char*)key);
}

/*
 * Hashes the selector using its name field.
 */
static inline uint32_t
_objc_selector_hash(struct objc_selector *sel)
{
	objc_assert(sel != NULL, "Can't hash NULL selector!");
	return objc_hash_string(sel->name);
}

static inline const char *
_objc_selector_get_types(struct objc_selector *sel)
{
	return sel->name + objc_strlen(sel->name) + 1;
}

/*
 * Uses direct page allocation for string allocation.
 *
 * Requires objc_selector_lock to be locked.
 */
static char *
_objc_selector_allocate_string(size_t size)
{
	/*
	 * No locking necessary, since this gets called
	 * only from places, where the objc_selector_lock
	 * is already locked.
	 */
	
	if (string_allocator_bytes_remaining < size){
		/*
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
		
		string_allocator = objc_alloc_page(M_SELECTOR_NAME_TYPE);
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

/*
 * Creates a new string including both name and types
 * and copies both strings over.
 *
 * Requires objc_selector_lock to be locked.
 */
static char *
_objc_selector_copy_name_and_types(const char *name, const char *types)
{
	size_t name_size = objc_strlen(name);
	size_t types_size = objc_strlen(types);
	size_t size = name_size + types_size + 2; /* +2 for two \0 */
	
	char *result = _objc_selector_allocate_string(size);
	
	memcpy(result, name, name_size + 1);
	memcpy(result + name_size + 1, types, types_size + 1);
	
	return result;
}

static inline SEL
_sel_allocate_sel_uid(void)
{
  static int objc_selector_counter = 1;
	int original_value = __sync_fetch_and_add(&objc_selector_counter, 1);
	return original_value;
}

/*
 * Inserts selector into the hashtable and sparse array.
 *
 * Requires objc_selector_lock to be locked.
 */
static BOOL
sel_registerName_direct(struct objc_selector *selector)
{
	objc_assert(selector != NULL, "Registering NULL selector!");
	
	if (selector->sel_uid != 0){
		/* Already registered */
		return NO;
	}
	
	struct objc_selector *registered_sel;
	registered_sel = objc_selector_table_get(objc_selector_hashtable,
						 selector->name);
	if (registered_sel != NULL){
		/* Just update the information in the original selector */
		selector->name = registered_sel->name;
		selector->sel_uid = registered_sel->sel_uid;
		return NO;
	}
	
	selector->sel_uid = _sel_allocate_sel_uid();
	
	if (objc_selector_insert(objc_selector_hashtable, selector) == 0){
		objc_log("Failed to insert selector %s!\n", selector->name);
		return NO;
	}
	
	objc_debug_log("Registering selector %s to sel_uid %d.\n",
		       selector->name, selector->sel_uid);
	
	SparseArrayInsert(objc_selector_sparse, selector->sel_uid, selector);
	
	return YES;
}

static inline SEL
_sel_register_name_no_lock(const char *name, const char *types)
{
	struct objc_selector *selector;
	selector = objc_selector_table_get(objc_selector_hashtable,
					   name);
	if (selector == NULL){
		/*
		 * Still NULL -> no other thread inserted it
		 * in the meanwhile.
		 */
		
		selector = objc_alloc(sizeof(struct objc_selector), M_SELECTOR_TYPE);
		selector->name =
		_objc_selector_copy_name_and_types(name, types);
		
		/* Will be populated when registered */
		selector->sel_uid = null_selector;
		
		if (selector->name == NULL){
			/* Probably ran out of memory */
			return null_selector;
		}
		
		if (!sel_registerName_direct(selector)){
			return null_selector;
		}
	}
	return selector->sel_uid;
}

/*
 * Assumes the lock is locked.
 *
 * Here is the issue: we have a objc_selector_reference structure pointer,
 * where the name is concatenated with types just as we need, however, the
 * actual sel_uid is there a pointer to the actual 16-bit int.
 *
 * The pointer points to a static variable which is then used in the module
 * for message sends, etc. This means that it is necessary to perform the
 * following steps:
 *
 * 1) See if a selector with this name already is registered. If yes, check
 *    if the types match. If not -> abort(), if yes, just update *sel_uid.
 * 2) If the selector is not registered, register it directly - add it to
 *    the hash table and get the UID, however, no copying should be
 *    performed. Also the *sel_uid must be updated.
 */
static inline void
_sel_register_selector_reference(struct objc_selector_reference * ref)
{
  struct objc_selector *selector;
	selector = objc_selector_table_get(objc_selector_hashtable,
                                     ref->selector_name);
  if (selector != NULL)
  {
    /* There already is a selector with this name! */
    const char *existing_sel_types = _objc_selector_get_types(selector);
    const char *new_sel_types = _objc_selector_get_types((struct objc_selector*)ref);
    BOOL typesEqual = objc_strings_equal(existing_sel_types, new_sel_types);
    objc_assert(typesEqual, "Registering selector %s with types %s failed as "
                "the same selector already exists with types %s!\n",
                ref->selector_name, new_sel_types, existing_sel_types);
    
    /* Update the UID. */
    *(ref->sel_uid) = selector->sel_uid;
  }
  else
  {
    /* Not found, register directly. */
    
    /* First, allocate the UID and assign it, clear the pointer. */
    SEL sel_uid = _sel_allocate_sel_uid();
    *(ref->sel_uid) = sel_uid;
    ref->sel_uid = NULL;
    
    /* Cast to selector struct. */
    selector = (struct objc_selector*)ref;
    selector->sel_uid = sel_uid;
    
    /* Insert into hash table. */
    if (objc_selector_insert(objc_selector_hashtable, selector) == 0){
      /* 
       * In this case, unlike user-inserted selectors we actually abort since
       * this may have fatal consequences (the same selector in multiple
       * modules could have different sel_uids).
       */
      objc_abort("Failed to insert selector %s!\n", selector->name);
    }
    
    objc_debug_log("Registering selector %s to sel_uid %d.\n",
                   selector->name, selector->sel_uid);
    
    SparseArrayInsert(objc_selector_sparse, selector->sel_uid, selector);
  }
}

static inline void
_sel_register_from_method_list_no_lock(objc_method_list *list)
{
	for (int i = 0; i < list->size; ++i){
		Method m = &list->list[i];
		m->selector = _sel_register_name_no_lock(m->selector_name,
							 m->selector_types);
	}
}


#pragma mark -
#pragma mark PUBLIC_FUNCTIONS

SEL
sel_registerName(const char *name, const char *types)
{
	objc_assert(name != NULL, "Cannot register a selector with NULL name!");
	objc_assert(types != NULL,
		    "Cannot register a selector with NULL types!");
	objc_assert(objc_strlen(types) > 2,
		    "Not enough types for registering selector.");
	
	struct objc_selector *selector;
	selector = objc_selector_table_get(objc_selector_hashtable, name);
	
	if (selector != NULL){
		const char *selector_types = _objc_selector_get_types(selector);
		objc_assert(objc_strings_equal(types, selector_types),
			    "Trying to register a selector (%s) with the same name"
			    " but different types (%s != %s)!\n",
                name,
                types,
                _objc_selector_get_types(selector));
		return selector->sel_uid;
	}
	
	/*
	 * Lock for rw access, try to look up again if some
	 * other thread hasn't inserted the selector and
	 * if not, allocate the structure and insert it.
	 */
	
	OBJC_LOCK_FOR_SCOPE(&objc_selector_lock);
	return _sel_register_name_no_lock(name, types);
}

const char *
sel_getName(SEL selector)
{
	if (selector == 0){
		return "((null))";
	}
	
	/*
	 * No locking necessary since we don't remove selectors
	 * from the run-time, which means the sparse array isn't
	 * going anywhere.
	 */
	
	struct objc_selector *sel_struct;
	sel_struct = (struct objc_selector *)
			SparseArrayLookup(objc_selector_sparse, selector);
	
	objc_assert(sel_struct != NULL,
		    "Trying to get name from an unregistered selector.");
	return sel_struct->name;
}

const char *
sel_getTypes(SEL selector)
{
	if (selector == 0){
		return "";
	}
	
	/*
	 * No locking necessary since we don't remove selectors
	 * from the run-time, which means the sparse array isn't
	 * going anywhere.
	 */
	
	struct objc_selector *sel_struct = (struct objc_selector *)
			SparseArrayLookup(objc_selector_sparse, selector);
	
	objc_assert(sel_struct != NULL,
		    "Trying to get types from an unregistered selector.");
	
	/* The types are in the same string as name, separated by \0 */
	return _objc_selector_get_types(sel_struct);
}

PRIVATE void
objc_register_selectors_from_method_list(objc_method_list *list)
{
	OBJC_LOCK_FOR_SCOPE(&objc_selector_lock);
	_sel_register_from_method_list_no_lock(list);
}

PRIVATE void
objc_register_selectors_from_class(Class cl, Class meta)
{
	OBJC_LOCK_FOR_SCOPE(&objc_selector_lock);
	
	objc_method_list *list = cl->methods;
	while (list != NULL){
		_sel_register_from_method_list_no_lock(list);
		list = list->next;
	}
	
	if (meta != Nil){
		list = meta->methods;
		while (list != NULL){
			_sel_register_from_method_list_no_lock(list);
			list = list->next;
		}
	}
}

PRIVATE void
objc_register_selector_array(struct objc_selector_reference *selectors,
			     unsigned int count)
{
	OBJC_LOCK_FOR_SCOPE(&objc_selector_lock);
	
  objc_debug_log("Registering selectors from array[%i]: %p\n", count, selectors);
  
	for (int i = 0; i < count; ++i){
    struct objc_selector_reference *selector = &selectors[i];
    
    _sel_register_selector_reference(selector);
    
    const char *name = selector->selector_name;
    const char *types = _objc_selector_get_types((struct objc_selector*)selector);
    
    objc_debug_log("Registering selector[%i]: %p\n", i, selector);
    objc_debug_log("\tName: %p (%s)\n", name, name);
    objc_debug_log("\tTypes: %p (%s)\n", types, types);
 		objc_debug_log("Registered as (SEL)%i\n",
                   (int)(((struct objc_selector*)selector)->sel_uid));
	}
}


#pragma mark -
#pragma mark INIT_FUNCTION

void
objc_selector_init(void)
{
	/* Assert that the runtime initialization lock is locked. */
	
	objc_debug_log("Initializing selectors.\n");
	
	/*
	 * Init the hash table that is used for getting selector
	 * name for SEL.
	 */
	objc_selector_hashtable =
		objc_selector_table_create(OBJC_SELECTOR_TABLE_INITIAL_CAPACITY);
	
	/*
	 * Init RW lock for locking the string allocator. The
	 * allocator itself is lazily allocated.
	 */
	objc_rw_lock_init(&objc_selector_lock, "objc_selector_lock");
	
	/*
	 * Sparse array holding the SEL -> Selector mapping.
	 */
	objc_selector_sparse = SparseArrayNew();
	
	BOOL cpu32_bit = sizeof(void*) == 4;
	const char *void_return_types = cpu32_bit ? "v8@0:4" : "v16@0:8";
	const char *object_return_types = cpu32_bit == 4 ? "@8@0:4" : "@16@0:8";
	
	objc_autorelease_selector = _sel_register_name_no_lock("autorelease",
						       object_return_types);
	objc_release_selector = _sel_register_name_no_lock("release",
							   void_return_types);
	objc_retain_selector = _sel_register_name_no_lock("retain",
							  object_return_types);
	objc_dealloc_selector = _sel_register_name_no_lock("dealloc",
							   void_return_types);
	objc_copy_selector = _sel_register_name_no_lock("copy",
							object_return_types);
	objc_cxx_destruct_selector = _sel_register_name_no_lock(".cxx_destruct",
							void_return_types);
	objc_cxx_construct_selector = _sel_register_name_no_lock(".cxx_construct",
							void_return_types);
	objc_load_selector = _sel_register_name_no_lock("load",
							void_return_types);
	objc_initialize_selector = _sel_register_name_no_lock("initialize",
						      void_return_types);
	objc_is_arc_compatible_selector = _sel_register_name_no_lock("_ARCCompliantRetainRelease",
								     void_return_types);
}


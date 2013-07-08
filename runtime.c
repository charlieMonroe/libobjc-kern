/*
 * This file implements initialization and setup functions of the run-time.
 */

#include "private.h" /* For *_inits - the public header is included here as well. */
#include "os.h" /* To figure out if the run-time is using inline functions or function pointers */

#include "structs/array.h" /* For default array implementation */
#include "structs/classhol.h" /* For default class holder imp */
#include "structs/selechol.h" /* For default selector holder imp */
#include "structs/cache.h" /* Default cache imp */

/**
 * This is marked during objc_init() as YES. After that point, no modifications
 * to the setup may be made.
 */
BOOL objc_runtime_has_been_initialized = NO;
BOOL objc_runtime_is_initializing = NO;

/**
 * Registering of initializers. As the run-time has no means of
 * allocation at this moment, a simple array is used. Static vars
 * are zero'ed, hence we just use the last NULL index.
 */
#define MAX_NUMBER_OF_INITIALIZERS 32
static objc_initializer_f objc_initializers[MAX_NUMBER_OF_INITIALIZERS];

void objc_runtime_register_initializer(objc_initializer_f initializer){
	int i;
	
	if (objc_runtime_is_initializing || objc_runtime_has_been_initialized){
		objc_abort("Cannot register an initializer when the run-time has already been initialized.");
	}
	
	for (i = 0; i < MAX_NUMBER_OF_INITIALIZERS; ++i){
		if (objc_initializers[i] == NULL){
			objc_initializers[i] = initializer;
			return;
		}
	}
	
	/** All initializer slots have been taken up. */
	objc_abort("All initializer slots are taken up!");
}

/**
 * Goes through registered initializers and calls them.
 */
static void _objc_runtime_perform_initializers(void){
	int i;
	for (i = 0; i < MAX_NUMBER_OF_INITIALIZERS; ++i){
		if (objc_initializers[i] == NULL){
			return;
		}else{
			objc_initializers[i]();
		}
	}
}


/* The exported runtime setup structure. Private for internal use only, though. */
objc_runtime_setup_t objc_setup;

/* See header for documentation */
void objc_runtime_set_setup(objc_runtime_setup_t *setup){
	if (OBJC_USES_INLINE_FUNCTIONS){
		objc_log("The run-time uses inline functions. Setting the function pointers has no effect.\n");
		return;
	}
	
	/**
	 * Check if either setup is NULL or the run-time has been already
	 * initialized - if so, we need to abort
	 */
	if (setup == NULL) {
		/* At this point, the abort function doesn't have to be set. */
		if (objc_abort != NULL){
			objc_abort("Cannot pass NULL setup!");
		}else{
			return;
		}
	}else if (objc_runtime_has_been_initialized == YES){
		/* abort is a required function */
		objc_abort("Cannot modify the run-time setup after it has "
			"been initialized");
	}
	
	/* Copy over everything */
	objc_setup = *setup;
}

/* See header for documentation */
void objc_runtime_get_setup(objc_runtime_setup_t *setup){
	if (OBJC_USES_INLINE_FUNCTIONS){
		objc_log("The run-time uses inline functions. No function pointers have been returned.\n");
		return;
	}
	
	if (setup != NULL){
		*setup = objc_setup;
	}
}


static int _objc_runtime_default_log(const char *format, ...){
	return 0;
}

#define objc_runtime_init_check_function_pointer(struct_path)\
	if (objc_setup.struct_path == NULL){\
		objc_setup.execution.abort("No function pointer set for " #struct_path "!\n");\
	}

#define objc_runtime_init_check_function_pointer_with_default_imp(struct_path, imp)\
	if (objc_setup.struct_path == NULL){\
		objc_setup.struct_path = imp;\
	}


/**
 * Validates all function pointers. Aborts the program if some of the required pointers isn't set.
 */
static void _objc_runtime_validate_function_pointers(void){
#if !OBJC_USES_INLINE_FUNCTIONS
	
	objc_runtime_init_check_function_pointer(execution.abort)
	
	objc_runtime_init_check_function_pointer(memory.allocator)
	objc_runtime_init_check_function_pointer(memory.deallocator)
	objc_runtime_init_check_function_pointer(memory.zero_allocator)
	
	objc_runtime_init_check_function_pointer(sync.rwlock.creator)
	objc_runtime_init_check_function_pointer(sync.rwlock.destroyer)
	objc_runtime_init_check_function_pointer(sync.rwlock.rlock)
	objc_runtime_init_check_function_pointer(sync.rwlock.unlock)
	objc_runtime_init_check_function_pointer(sync.rwlock.wlock)
	
	objc_runtime_init_check_function_pointer_with_default_imp(logging.log, _objc_runtime_default_log)
	
	objc_runtime_init_check_function_pointer_with_default_imp(array.creator, array_create)
	objc_runtime_init_check_function_pointer_with_default_imp(array.append, array_add)
	objc_runtime_init_check_function_pointer_with_default_imp(array.enum_getter, array_get_enumerator)
	
	objc_runtime_init_check_function_pointer_with_default_imp(class_holder.creator, class_holder_create)
	objc_runtime_init_check_function_pointer_with_default_imp(class_holder.inserter, class_holder_insert_class)
	objc_runtime_init_check_function_pointer_with_default_imp(class_holder.lookup, class_holder_lookup_class)
	
	objc_runtime_init_check_function_pointer_with_default_imp(selector_holder.creator, selector_holder_create)
	objc_runtime_init_check_function_pointer_with_default_imp(selector_holder.inserter, selector_holder_insert_selector)
	objc_runtime_init_check_function_pointer_with_default_imp(selector_holder.lookup, selector_holder_lookup_selector)

	objc_runtime_init_check_function_pointer_with_default_imp(cache.creator, cache_create)
	objc_runtime_init_check_function_pointer_with_default_imp(cache.destroyer, cache_destroy)
	objc_runtime_init_check_function_pointer_with_default_imp(cache.fetcher, cache_fetch)
	objc_runtime_init_check_function_pointer_with_default_imp(cache.inserter, cache_insert)
	
#endif /* OBJC_USES_INLINE_FUNCTIONS */
}

/* See header for documentation */
void objc_runtime_init(void){
	if (objc_runtime_is_initializing || objc_runtime_has_been_initialized){
		/* Make sure that we don't initialize twice */
		return;
	}
	
	/* Run-time has been initialized */
	objc_runtime_is_initializing = YES;
	
	_objc_runtime_perform_initializers();
	
	/* If inline functions aren't in use, check the function pointers */
	if (!OBJC_USES_INLINE_FUNCTIONS){
		_objc_runtime_validate_function_pointers();
	}
	
	/* Initialize inner structures */
	objc_selector_init();
	objc_class_init();
	objc_install_base_classes();
	
	objc_runtime_has_been_initialized = YES;
	objc_runtime_is_initializing = NO;
}

/********** Getters and setters. ***********/
/*
 * A macro that creates the getter and setter function bodies. The type argument
 * takes in the function type (e.g. objc_allocator), the name argument is used
 * in the function name (both type and name arguments need to match the decls
 * in the header!), struct_path is the path in the objc_runtime_setup_struct
 * structure - for functions that are directly part of the structure, pass their
 * name, otherwise you need to include the sub-structure they're in as well,
 * e.g. 'class_holder.creator'.
 */
#define objc_runtime_create_getter_setter_function_body(type, name, struct_path)\
	void objc_runtime_set_##name(type name){\
		if (OBJC_USES_INLINE_FUNCTIONS){\
			return;\
		}\
		if (objc_runtime_has_been_initialized){\
			objc_abort("Cannot modify the run-time " #name " after the "\
				 				"run-time has been initialized");\
		}\
		objc_setup.struct_path = name;\
	}\
	type objc_runtime_get_##name(void){ \
		if (OBJC_USES_INLINE_FUNCTIONS){\
			objc_log("The run-time uses inline functions. NULL pointer has been returned.\n");\
			return NULL;\
		}\
		return objc_setup.struct_path; \
	}

objc_runtime_create_getter_setter_function_body(objc_abort_f, abort, execution.abort)
objc_runtime_create_getter_setter_function_body(objc_allocator_f, allocator, memory.allocator)
objc_runtime_create_getter_setter_function_body(objc_deallocator_f, deallocator, memory.deallocator)
objc_runtime_create_getter_setter_function_body(objc_zero_allocator_f, zero_allocator, memory.zero_allocator)
objc_runtime_create_getter_setter_function_body(objc_class_holder_creator_f, class_holder_creator, class_holder.creator)
objc_runtime_create_getter_setter_function_body(objc_class_holder_inserter_f, class_holder_inserter, class_holder.inserter)
objc_runtime_create_getter_setter_function_body(objc_class_holder_lookup_f, class_holder_lookup, class_holder.lookup)

objc_runtime_create_getter_setter_function_body(objc_selector_holder_creator_f, selector_holder_creator, selector_holder.creator)
objc_runtime_create_getter_setter_function_body(objc_selector_holder_inserter_f, selector_holder_inserter, selector_holder.inserter)
objc_runtime_create_getter_setter_function_body(objc_selector_holder_lookup_f, selector_holder_lookup, selector_holder.lookup)

objc_runtime_create_getter_setter_function_body(objc_log_f, log, logging.log)

objc_runtime_create_getter_setter_function_body(objc_rw_lock_creator_f, rw_lock_creator, sync.rwlock.creator)
objc_runtime_create_getter_setter_function_body(objc_rw_lock_destroyer_f, rw_lock_destroyer, sync.rwlock.destroyer)
objc_runtime_create_getter_setter_function_body(objc_rw_lock_read_lock_f, rw_lock_rlock, sync.rwlock.rlock)
objc_runtime_create_getter_setter_function_body(objc_rw_lock_write_lock_f, rw_lock_wlock, sync.rwlock.wlock)
objc_runtime_create_getter_setter_function_body(objc_rw_lock_unlock_f, rw_lock_unlock, sync.rwlock.unlock)

objc_runtime_create_getter_setter_function_body(objc_array_creator_f, array_creator, array.creator)
objc_runtime_create_getter_setter_function_body(objc_array_enumerator_getter_f, array_enumerator_getter, array.enum_getter)
objc_runtime_create_getter_setter_function_body(objc_array_append_f, array_append, array.append)

objc_runtime_create_getter_setter_function_body(objc_cache_creator_f, cache_creator, cache.creator)
objc_runtime_create_getter_setter_function_body(objc_cache_mark_to_dealloc_f, cache_destroyer, cache.destroyer)
objc_runtime_create_getter_setter_function_body(objc_cache_fetcher_f, cache_fetcher, cache.fetcher)
objc_runtime_create_getter_setter_function_body(objc_cache_inserter_f, cache_inserter, cache.inserter)


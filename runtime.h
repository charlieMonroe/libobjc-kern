/*
 * This file contains functions and structures that are meant for the run-time
 * setup and initialization.
 */

#ifndef OBJC_RUNTIME_H_
#define OBJC_RUNTIME_H_

#include "ftypes.h"

/**
 * Structures that defines the run-time setup. This means all functions that
 * the run-time needs for running and that can be invoked.
 *
 * Note that some function pointers that are marked as optional may be NULL.
 */

typedef struct {
	objc_allocator_f allocator;
	objc_deallocator_f deallocator;
	objc_zero_allocator_f zero_allocator;
} objc_setup_memory_t;

typedef struct {
	objc_abort_f abort;
} objc_setup_execution_t;

typedef struct {
	objc_class_holder_creator_f creator;
	objc_class_holder_inserter_f inserter;
	objc_class_holder_lookup_f lookup;
} objc_setup_class_holder_t;

typedef struct {
	objc_selector_holder_creator_f creator;
	objc_selector_holder_inserter_f inserter;
	objc_selector_holder_lookup_f lookup;
} objc_setup_selector_holder_t;

typedef struct {
	objc_log_f log;
} objc_setup_logging_t;

typedef struct {
	objc_rw_lock_creator_f creator;
	objc_rw_lock_destroyer_f destroyer;
	objc_rw_lock_read_lock_f rlock;
	objc_rw_lock_write_lock_f wlock;
	objc_rw_lock_unlock_f unlock;
} objc_setup_rw_lock_t;

typedef struct {
	objc_setup_rw_lock_t rwlock;
} objc_setup_sync_t;

typedef struct {
	objc_array_creator_f creator;
	objc_array_enumerator_getter_f enum_getter;
	objc_array_append_f append;
} objc_setup_array_t;

typedef struct {
	objc_cache_creator_f creator;
	objc_cache_mark_to_dealloc_f destroyer;
	objc_cache_fetcher_f fetcher;
	objc_cache_inserter_f inserter;
} objc_setup_cache_t;

typedef struct {
	objc_setup_memory_t memory;
	objc_setup_execution_t execution;
	objc_setup_sync_t sync;
	
	/**
	 * Logging is optional. If the logging function
	 * is not available, the run-time will simply not log anything.
	 */
	objc_setup_logging_t logging;
	
	/**
	 * The following pointers are to be set optionally
	 * as this run-time provides a default implementation.
	 * If, however, you modify one pointer of the set,
	 * all other pointers need to be modified as well...
	 */
	objc_setup_class_holder_t class_holder;
	objc_setup_selector_holder_t selector_holder;
	objc_setup_array_t array;
	objc_setup_cache_t cache;
	
} objc_runtime_setup_t;


/**
 * This function copies over all the function pointers. If you want to set just
 * a few of the pointers, consider using one of the functions below instead.
 *
 * If the run-time has already been initialized, calling this function aborts
 * execution of the program.
 */
extern void objc_runtime_set_setup(objc_runtime_setup_t *setup);

/**
 * Copies over all the current function pointers into the setup structure.
 */
extern void objc_runtime_get_setup(objc_runtime_setup_t *setup);

/**
 * Initializers and registering.
 *
 * If you have a function that should modify the run-time somehow before
 * it gets initialized, register an initializer using the objc_runtime_register_initializer
 * function.
 *
 * Initializers get performed in the same order as they get registered.
 */
typedef void(*objc_initializer_f)(void);
extern void objc_runtime_register_initializer(objc_initializer_f initializer);


/**
 * This function initializes the run-time, checks for any missing function pointers,
 * fills in the built-in function pointers and seals the run-time setup.
 */
extern void objc_runtime_init(void);


/**
 * All the following functions serve as getters and setters for the functions
 * within the setup structure.
 */

/* A macro for creating function declarations */
#define objc_runtime_create_getter_setter_function_decls(type, name)\
	 extern void objc_runtime_set_##name(type name);\
	 extern type objc_runtime_get_##name(void);

objc_runtime_create_getter_setter_function_decls(objc_abort_f, abort)
objc_runtime_create_getter_setter_function_decls(objc_allocator_f, allocator)
objc_runtime_create_getter_setter_function_decls(objc_deallocator_f, deallocator)
objc_runtime_create_getter_setter_function_decls(objc_zero_allocator_f, zero_allocator)
objc_runtime_create_getter_setter_function_decls(objc_class_holder_creator_f, class_holder_creator)
objc_runtime_create_getter_setter_function_decls(objc_class_holder_lookup_f, class_holder_lookup)

objc_runtime_create_getter_setter_function_decls(objc_selector_holder_creator_f, selector_holder_creator)
objc_runtime_create_getter_setter_function_decls(objc_selector_holder_lookup_f, selector_holder_lookup)

objc_runtime_create_getter_setter_function_decls(objc_log_f, log)

objc_runtime_create_getter_setter_function_decls(objc_rw_lock_creator_f, rw_lock_creator)
objc_runtime_create_getter_setter_function_decls(objc_rw_lock_destroyer_f, rw_lock_destroyer)
objc_runtime_create_getter_setter_function_decls(objc_rw_lock_read_lock_f, rw_lock_rlock)
objc_runtime_create_getter_setter_function_decls(objc_rw_lock_write_lock_f, rw_lock_wlock)
objc_runtime_create_getter_setter_function_decls(objc_rw_lock_unlock_f, rw_lock_unlock)

objc_runtime_create_getter_setter_function_decls(objc_array_creator_f, array_creator)
objc_runtime_create_getter_setter_function_decls(objc_array_enumerator_getter_f, array_enumerator_getter)
objc_runtime_create_getter_setter_function_decls(objc_array_append_f, array_append)

objc_runtime_create_getter_setter_function_decls(objc_cache_creator_f, cache_creator)
objc_runtime_create_getter_setter_function_decls(objc_cache_mark_to_dealloc_f, cache_destroyer)
objc_runtime_create_getter_setter_function_decls(objc_cache_fetcher_f, cache_fetcher)
objc_runtime_create_getter_setter_function_decls(objc_cache_inserter_f, cache_inserter)

#endif /* OBJC_RUNTIME_H_ */

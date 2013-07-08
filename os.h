/**
 * This header file contains various declarations of functions, depending
 * on whether the run-time is using inline functions, or function pointers.
 *
 * If function pointers are used, the user, or dynamic loader, or other deity
 * is responsible for initializing the run-time's setup.
 *
 * If inline functions are used, the person compiling this run-time must
 * supply a header file for his OS, that implements inline functions with
 * such names as are the defines in the function-pointer section, and
 * with the same signature as the function types defined in function-types.h
 */

#ifndef OBJC_OS_H_
#define OBJC_OS_H_

/** Try to detect C89 automatically. */
#if !defined(USES_C89) && !defined(__GNUC_STDC_INLINE__)
	#define USES_C89 1
#endif

/** C89 doesn't have static inline functions. */
#if !defined(USES_C89)
	#define OBJC_INLINE static inline
#else
	#define OBJC_INLINE static
#endif

/** 
 * Some compilers might not support this attribute
 * that forces the compiler to always inline the functions.
 */
#if defined(HAS_ALWAYS_INLINE_ATTRIBUTE)
	#define OBJC_ALWAYS_INLINE __attribute__((always_inline))
#else
	#define OBJC_ALWAYS_INLINE
#endif

/* Fallback to false. */
#if !defined(OBJC_USES_INLINE_FUNCTIONS)
	#define OBJC_USES_INLINE_FUNCTIONS 1
#endif

#if OBJC_USES_INLINE_FUNCTIONS

	/********* INLINE FUNCTIONS *********/

	#if 1
		#include "extras/inline.h"
	/* #elseif defined(TARGET_MY_OS)
		#include "os-my-os.h" */
	#else
		#error "This OS isn't supported at the moment."
	#endif


#else

	/********* FUNCTION POINTERS *********/

	#include "private.h"

	/* Memory */
	#define objc_alloc objc_setup.memory.allocator
	#define objc_zero_alloc objc_setup.memory.zero_allocator
	#define objc_dealloc objc_setup.memory.deallocator

	/* Execution */
	#define objc_abort objc_setup.execution.abort

	/* Logging */
	#define objc_log objc_setup.logging.log

	/* RW lock */
	#define objc_rw_lock_create objc_setup.sync.rwlock.creator
	#define objc_rw_lock_rlock objc_setup.sync.rwlock.rlock
	#define objc_rw_lock_wlock objc_setup.sync.rwlock.wlock
	#define objc_rw_lock_unlock objc_setup.sync.rwlock.unlock
	#define objc_rw_lock_destroy objc_setup.sync.rwlock.destroyer

	/* Class holder */
	#define objc_class_holder_create objc_setup.class_holder.creator
	#define objc_class_holder_insert objc_setup.class_holder.inserter
	#define objc_class_holder_lookup objc_setup.class_holder.lookup

	/* Selector holder */
	#define objc_selector_holder_create objc_setup.selector_holder.creator
	#define objc_selector_holder_insert objc_setup.selector_holder.inserter
	#define objc_selector_holder_lookup objc_setup.selector_holder.lookup

	/* Array */
	#define objc_array_create objc_setup.array.creator
	#define objc_array_append objc_setup.array.append
	#define objc_array_get_enumerator objc_setup.array.enum_getter

	/* Cache */
	#define objc_cache_create objc_setup.cache.creator
	#define objc_cache_destroy objc_setup.cache.destroyer
	#define objc_cache_fetch objc_setup.cache.fetcher
	#define objc_cache_insert objc_setup.cache.inserter

#endif

#endif /* OBJC_OS_H_ */

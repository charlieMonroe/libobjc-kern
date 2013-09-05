#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "runtime.h"
#include "init.h"

/*
 * This is marked during objc_init() as YES. After that point, no modifications
 * to the setup may be made.
 */
PRIVATE BOOL objc_runtime_initialized = NO;

PRIVATE unsigned int objc_lock_count = 0;
PRIVATE unsigned int objc_lock_destroy_count = 0;
PRIVATE unsigned int objc_lock_locked_count = 0;

/* See header for documentation */

void objc_runtime_init(void) {
	if (objc_runtime_initialized){
		/* Make sure that we don't initialize twice */
		objc_debug_log("Trying to initialize runtime for the second "
			       "time.\n");
		return;
	}
	
	objc_debug_log("Initializing runtime.\n");
	
	objc_rw_lock_init(&objc_runtime_lock, "objc_runtime_lock");
	
	/* Initialize inner structures */
	objc_selector_init();
	objc_dispatch_tables_init();
	objc_class_init();
	objc_arc_init();
	objc_protocol_init();
	objc_associated_objects_init();
	objc_exceptions_init();
	objc_string_allocator_init();
	
	/* Do NOT call blocks init. They get initialized in KKObject's load! */
	/* objc_blocks_init(); */
	
	objc_runtime_initialized = YES;
	
	/* 
	 * For debugging purposes, when one needs to enter some breakpoints in the
	 * kernel debugger.
	 */
	// pause("objc_debug", 1000);
}

void	objc_runtime_destroy(void) {
	if (!objc_runtime_initialized) {
		objc_log("Cannot destroy runtime that hasn't been initialized!\n");
		return;
	}
	
	objc_debug_log("Destroying runtime.\n");
	
	/* Call destroys to all other modules. */
	objc_selector_destroy();
	objc_class_destroy();
	objc_protocol_destroy();
	objc_dispatch_tables_destroy();
	objc_arc_destroy();
	objc_associated_objects_destroy();
	objc_exceptions_destroy();
	objc_blocks_destroy();
	objc_string_allocator_destroy();

	/* The lock must be destroyed at the end since some destructors may used it */
	objc_rw_lock_destroy(&objc_runtime_lock);
	
}



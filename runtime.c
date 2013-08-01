
#include "runtime.h"
#include "types.h"
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

void objc_runtime_init(void){
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
	objc_exceptions_init();
	
	objc_runtime_initialized = YES;
}



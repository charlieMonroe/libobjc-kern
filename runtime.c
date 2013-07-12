
#include "runtime.h"
#include "os.h"
#include "types.h"
#include "private.h"

/**
 * This is marked during objc_init() as YES. After that point, no modifications
 * to the setup may be made.
 */
BOOL objc_runtime_has_been_initialized = NO;
BOOL objc_runtime_is_initializing = NO;


/* See header for documentation */

__attribute__((constructor))
void objc_runtime_init(void){
	if (objc_runtime_is_initializing || objc_runtime_has_been_initialized){
		/* Make sure that we don't initialize twice */
		return;
	}
	
	/* Run-time has been initialized */
	objc_runtime_is_initializing = YES;
	
	/* Initialize inner structures */
	objc_selector_init();
	init_dispatch_tables();
	objc_class_init();
	objc_install_base_classes();
	
	objc_runtime_has_been_initialized = YES;
	objc_runtime_is_initializing = NO;
}



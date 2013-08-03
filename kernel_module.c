#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/linker.h>

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "init.h"


static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		objc_runtime_init();
		
		/* Attempt to load the runtime's basic classes. */
		struct linker_file *file = module_file(module);
		objc_debug_log("Gotten module file %p (%s)\n", file, file->filename);
			
		caddr_t objc_module = linker_file_lookup_symbol(file,
								".objc_module",
								TRUE);
		objc_debug_log("Gotten address of the objc_module %p\n", objc_module);
		break;
	case MOD_UNLOAD:
		objc_runtime_destroy();
		uprintf("G'bye\n");
		break;
	default:
		e = EOPNOTSUPP;
		break;
	}
	return (e);
}

static moduledata_t libobjc_conf = {
	"libobjc", 	/* Module name. */
	event_handler,  /* Event handler. */
	NULL 		/* Extra data */
};

DECLARE_MODULE(libobjc, libobjc_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);

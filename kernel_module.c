#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/linker.h>

#include <sys/elf.h>

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "init.h"
#include "loader.h"
#include "utils.h"

SET_DECLARE(objc_module_list_set, struct objc_loader_module);

extern void handmade_class_test(void);

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		/* Attempt to load the runtime's basic classes. */
		if (SET_COUNT(objc_module_list_set) == 0) {
			objc_log("Error loading libobj - cannot load"
				" basic required classes.");
			break;
		}
		_objc_load_modules(SET_BEGIN(objc_module_list_set),
						   SET_LIMIT(objc_module_list_set));

		handmade_class_test();
		
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

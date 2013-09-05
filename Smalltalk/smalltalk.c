#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include "../os.h"
#include <kernobjc/runtime.h>

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
		case MOD_LOAD:
			_objc_load_kernel_module(module);
			break;
		case MOD_UNLOAD:
			if (!_objc_unload_kernel_module(module)){
				e = EOPNOTSUPP;
			}
			break;
		default:
			e = EOPNOTSUPP;
			break;
	}
	return (e);
}

static moduledata_t smalltalk_conf = {
	"Smalltalk Runtime", 	/* Module name. */
	event_handler,  /* Event handler. */
	NULL 		/* Extra data */
};

DECLARE_MODULE(smalltalk_runtime, smalltalk_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
MODULE_VERSION(smalltalk_runtime, 0);

/* Depend on libobjc */
MODULE_DEPEND(smalltalk_runtime, libobjc, 0, 0, 999);

/* Depend on Foundation */
MODULE_DEPEND(smalltalk_runtime, objc_foundation, 0, 0, 999);

/* Depend on LanguageKit */
MODULE_DEPEND(smalltalk_runtime, objc_langkit, 0, 0, 999);

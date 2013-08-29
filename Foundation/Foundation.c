#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/linker.h>
#include <sys/limits.h>

#include "../os.h"
#include "../kernobjc/types.h"
#include "../loader.h"

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
		case MOD_LOAD:
			_objc_load_kernel_module(module);
			break;
		case MOD_UNLOAD:
			/* Nothing really - maybe some cleanup in the future. */
			break;
		default:
			e = EOPNOTSUPP;
			break;
	}
	return (e);
}

static moduledata_t foundation_conf = {
	"Foundation", 	/* Module name. */
	event_handler,  /* Event handler. */
	NULL 		/* Extra data */
};

DECLARE_MODULE(objc_foundation, foundation_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
MODULE_VERSION(objc_foundation, 0);



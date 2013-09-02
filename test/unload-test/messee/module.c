#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/linker.h>
#include <sys/limits.h>

#include "../../../os.h"
#include "../../../kernobjc/types.h"
#include "../../../loader.h"

extern void run_test(void);

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
		case MOD_LOAD:
			_objc_load_kernel_module(module);
			run_test();
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

static moduledata_t messee_conf = {
	"messee", 	/* Module name. */
	event_handler,  /* Event handler. */
	NULL 		/* Extra data */
};

DECLARE_MODULE(messee, messee_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
MODULE_VERSION(messee, 0);

/* Depend on libobjc */
MODULE_DEPEND(messee, libobjc, 0, 0, 999);

/* Depend on myclass */
MODULE_DEPEND(messee, myclass, 0, 0, 999);

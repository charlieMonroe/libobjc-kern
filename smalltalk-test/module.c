#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

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

static moduledata_t smalltalk_test_conf = {
	"smalltalk_test", 	/* Module name. */
	event_handler,  /* Event handler. */
	NULL 		/* Extra data */
};

DECLARE_MODULE(smalltalk_test, smalltalk_test_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
MODULE_VERSION(smalltalk_test, 0);

MODULE_DEPEND(smalltalk_test, libobjc, 0, 0, 999);
MODULE_DEPEND(smalltalk_test, objc_foundation, 0, 0, 999);
MODULE_DEPEND(smalltalk_test, objc_langkit, 0, 0, 999);
MODULE_DEPEND(smalltalk_test, smalltalk_runtime, 0, 0, 999);

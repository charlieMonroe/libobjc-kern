
#ifdef _KERNEL
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/linker.h>
#include <sys/limits.h>
#endif

#include "../kernobjc/runtime.h"
#include "../types.h"
#include "../loader.h"

void run_array_test(void);
void run_dictionary_test(void);

void run_tests(void);
void run_tests(void)
{
	run_array_test();
	run_dictionary_test();
}

#ifdef _KERNEL

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
		case MOD_LOAD:
			/* Attempt to load this module's classes. */
			_objc_load_kernel_module(module);
			
			run_tests();
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

static moduledata_t foundation_test_conf = {
	"foundation_test", 	/* Module name. */
	event_handler,  /* Event handler. */
	NULL 		/* Extra data */
};

DECLARE_MODULE(foundation_test, foundation_test_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
MODULE_VERSION(foundation_test, 0);

/* Depend on libobjc */
MODULE_DEPEND(foundation_test, libobjc, 0, 0, 999);

/* Depend on Foundation */
MODULE_DEPEND(foundation_test, objc_foundation, 0, 0, 999);

#endif

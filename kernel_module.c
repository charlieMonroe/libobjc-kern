#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "init.h"


static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		objc_runtime_init();
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

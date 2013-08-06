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


struct mod {
	const char *name;
	void *symtab;
	int version;
};




extern void associated_objects_test(void);

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		objc_runtime_init();
		
		/* Attempt to load the runtime's basic classes. */
		struct linker_file *file = module_file(module);
		objc_debug_log("Gotten module file %p (%s)\n", file, file->filename);
			
		struct common_symbol *symbol = file->common.stqh_first;
		while (symbol != NULL && STAILQ_NEXT(symbol, link) != NULL){
			objc_debug_log("\t%p  ->  %s\n", symbol->address, symbol->name);
			symbol = STAILQ_NEXT(symbol, link);
		}

		struct module *m = file->modules.tqh_first;
		while (m != NULL && module_getfnext(m) != NULL){
			objc_debug_log("\t%p -> %s\n", m, module_getname(m));
			m = module_getfnext(m);
		}

/*		caddr_t objc_module = linker_file_lookup_symbol(file,
								".objc_module"
								TRUE);
		objc_debug_log("Gotten address of the objc_module %p\n", objc_module);
		_objc_load_module((struct objc_loader_module*)objc_module);
		associated_objects_test();*/
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

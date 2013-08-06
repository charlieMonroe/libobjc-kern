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

struct mod {
	const char *name;
	void *symtab;
	int version;
};

// SET_DECLARE(objc_module_list_set, struct mod);
extern struct mod __start_set_objc_module_list_set;
extern struct mod __stop_set_objc_module_list_set;

extern void associated_objects_test(void);

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
	case MOD_LOAD:
		objc_runtime_init();
		
		/* Attempt to load the runtime's basic classes. */
		struct linker_file *file = module_file(module);
		objc_debug_log("Gotten module file %p (%s)\n", file, file->filename);
					
		caddr_t objc_module = linker_file_lookup_symbol(file,
								".objc_module_list",
								FALSE);
		objc_debug_log("Gotten address of the module list - %p\n", objc_module);

		struct mod *start = &__start_set_objc_module_list_set;
		struct mod *end = &__stop_set_objc_module_list_set;

		unsigned int struct_size = sizeof(struct mod);
		
		// The struct size must be sizeof(void*)-aligned
		if ((struct_size % sizeof(void*)) != 0){
			struct_size += sizeof(void*) - (struct_size % sizeof(void*));
		}
		unsigned int count = ((((size_t)end) - ((size_t)start)) / struct_size);
		
		objc_debug_log("Start %p, end %p, sizeof(mod) = %d\n", start, end, (unsigned)sizeof(struct mod));
		objc_debug_log("Module count: %d\n", count);
		
		for (int i = 0; i < count; ++i){
			struct mod *m = &start[i];
			objc_debug_log("Module %p\n", m);
			objc_debug_log("\t\t->name %p\n", m->name);
			objc_debug_log("\t\t->symtab %p\n", m->symtab);
			objc_debug_log("\t\t->version %x\n", m->version);

			if (((size_t)m->name) > 5){
				objc_debug_log("Trying to resolve name: %s\n", m->name);
			}

		}		
//		struct mod *modules = __start_objc_module_list;
//		for (int i = 0; i < 3; ++i){
//			struct mod *m = &modules[i];
//			objc_debug_log("Gotten address of the module %p -> %s\n", m, m->name);
//		}


/*		objc_debug_log("Gotten address of the objc_module %p\n", objc_module);
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

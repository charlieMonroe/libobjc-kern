#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/linker.h>
#include <sys/limits.h>

#include <sys/elf.h>

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "init.h"
#include "loader.h"
#include "utils.h"

SET_DECLARE(objc_module_list_set, struct objc_loader_module);

// extern void run_tests(void);


static void get_elf(struct module *module){
	Elf_Ehdr *hdr = (Elf_Ehdr *)preload_search_info(module, MODINFO_METADATA |
													MODINFOMD_ELFHDR);
	objc_log("Found ELF header %p\n", hdr);
	if (hdr == NULL){
		return;
	}
	
	objc_log("Section headers start at offset %p, number of sections %d\n",
			 (void*)hdr->e_shoff,
			 (unsigned)hdr->e_shnum);
	objc_log("Program headers start at offset %p, number of sections %d\n",
			 (void*)hdr->e_phoff,
			 (unsigned)hdr->e_phnum);
}

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
		// _objc_load_modules(SET_BEGIN(objc_module_list_set),
						   SET_LIMIT(objc_module_list_set));

		// run_tests();
		
		get_elf(module);
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
MODULE_VERSION(libobjc, 0);
MODULE_DEPEND(libobjc, libunwind, 0, INT_MAX, 0);


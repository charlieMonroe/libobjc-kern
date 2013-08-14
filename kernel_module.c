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
	linker_file_t file = module_file(module);
	Elf_Phdr *phdr = (Elf_Phdr *)file->address;
	objc_log("Found ELF program header %p\n", phdr);
	if (hdr == NULL){
		return;
	}


	
	objc_log("PHDR dump:\n");
	objc_log("\tp_type: \t\t%lu\n", phdr->p_type);
	objc_log("\tp_flags: \t\t%lu\n", phdr->p_flags);
	objc_log("\tp_offset: \t\t0x%lx\n", phdr->p_offset);
	objc_log("\tp_vaddr: \t\t0x%lx\n", phdr->p_vaddr);
	objc_log("\tp_paddr: \t\t0x%lx\n", phdr->p_paddr);
	objc_log("\tp_filesz: \t\t%lu\n", phdr->p_filesz);
	objc_log("\tp_memsz: \t\t%lu\n", phdr->p_memsz);
	objc_log("\tp_align: \t\t%lu\n", phdr->p_align);
	
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
		//				   SET_LIMIT(objc_module_list_set));

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


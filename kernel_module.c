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

#include <sys/namei.h>
#include <sys/fcntl.h>
#include <sys/syscallsubr.h>
#include <sys/linker.h>

SET_DECLARE(objc_module_list_set, struct objc_loader_module);

// extern void run_tests(void);




static void get_elf(struct module *module){
	linker_file_t file = module_file(module);
	
	int fd = kern_open(curthread, file->pathname, UIO_SYSSPACE, O_RDONLY, 0);
	
	caddr_t firstpage = malloc(PAGE_SIZE, M_LINKER, M_WAITOK);
	struct uio auio;
	struct iovec aiov;
	aiov.iov_base = firstpage;
	aiov.iov_len = PAGE_SIZE;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = PAGE_SIZE;
	auio.uio_segflg = UIO_SYSSPACE;
	
	kern_readv(curthread, fd, &auio);
	
	Elf_Ehdr *ehdr = (Elf_Ehdr *)firstpage;
	objc_log("EHDR dump:\n");
	objc_log("\te_type: \t\t%lx\n", (unsigned long)ehdr->e_type);
	objc_log("\te_machine: \t\t%lx\n", (unsigned long)ehdr->e_machine);
	objc_log("\te_version: \t\t0x%lx\n", (unsigned long)ehdr->e_version);
	objc_log("\te_entry: \t\t0x%lx\n", (unsigned long)ehdr->e_entry);
	objc_log("\te_phoff: \t\t0x%lx\n", (unsigned long)ehdr->e_phoff);
	objc_log("\te_shoff: \t\t%lx\n", (unsigned long)ehdr->e_shoff);
	objc_log("\te_flags: \t\t%lu\n", (unsigned long)ehdr->e_flags);
	objc_log("\te_ehsize: \t\t%lu\n", (unsigned long)ehdr->e_ehsize);
	objc_log("\te_phentsize: \t\t%lu\n", (unsigned long)ehdr->e_phentsize);
	objc_log("\te_phnum: \t\t%lu\n", (unsigned long)ehdr->e_phnum);
	objc_log("\te_shentsize: \t\t%lu\n", (unsigned long)ehdr->e_shentsize);
	objc_log("\te_shnum: \t\t%lu\n", (unsigned long)ehdr->e_shnum);
	objc_log("\te_shstrndx: \t\t%lu\n", (unsigned long)ehdr->e_shstrndx);
	
	
	/*	elf_file_t file = (elf_file_t)module_file(module);
	
	objc_log("ELF file dump (%p):\n", file);
	objc_log("\tpreloaded: \t\t%i\n", file->preloaded);
	objc_log("\taddress: \t\t%p\n", file->address);
	objc_log("\tddbsymtab: \t\t%p\n", file->ddbsymtab);
*/

/*
	objc_log("PHDR dump:\n");
	objc_log("\tp_type: \t\t%u\n", phdr->p_type);
	objc_log("\tp_flags: \t\t%u\n", phdr->p_flags);
	objc_log("\tp_offset: \t\t0x%lx\n", phdr->p_offset);
	objc_log("\tp_vaddr: \t\t0x%lx\n", phdr->p_vaddr);
	objc_log("\tp_paddr: \t\t0x%lx\n", phdr->p_paddr);
	objc_log("\tp_filesz: \t\t%lu\n", phdr->p_filesz);
	objc_log("\tp_memsz: \t\t%lu\n", phdr->p_memsz);
	objc_log("\tp_align: \t\t%lu\n", phdr->p_align);
*/	
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


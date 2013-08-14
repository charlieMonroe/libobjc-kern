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

typedef struct elf_file {
	struct linker_file lf;		/* Common fields */
	int		preloaded;	/* Was file pre-loaded */
	caddr_t		address;	/* Relocation address */
#ifdef SPARSE_MAPPING
	vm_object_t	object;		/* VM object to hold file pages */
#endif
	Elf_Dyn		*dynamic;	/* Symbol table etc. */
	Elf_Hashelt	nbuckets;	/* DT_HASH info */
	Elf_Hashelt	nchains;
	const Elf_Hashelt *buckets;
	const Elf_Hashelt *chains;
	caddr_t		hash;
	caddr_t		strtab;		/* DT_STRTAB */
	int		strsz;		/* DT_STRSZ */
	const Elf_Sym	*symtab;		/* DT_SYMTAB */
	Elf_Addr	*got;		/* DT_PLTGOT */
	const Elf_Rel	*pltrel;	/* DT_JMPREL */
	int		pltrelsize;	/* DT_PLTRELSZ */
	const Elf_Rela	*pltrela;	/* DT_JMPREL */
	int		pltrelasize;	/* DT_PLTRELSZ */
	const Elf_Rel	*rel;		/* DT_REL */
	int		relsize;	/* DT_RELSZ */
	const Elf_Rela	*rela;		/* DT_RELA */
	int		relasize;	/* DT_RELASZ */
	caddr_t		modptr;
	const Elf_Sym	*ddbsymtab;	/* The symbol table we are using */
	long		ddbsymcnt;	/* Number of symbols */
	caddr_t		ddbstrtab;	/* String table */
	long		ddbstrcnt;	/* number of bytes in string table */
	caddr_t		symbase;	/* malloc'ed symbold base */
	caddr_t		strbase;	/* malloc'ed string base */
	caddr_t		ctftab;		/* CTF table */
	long		ctfcnt;		/* number of bytes in CTF table */
	caddr_t		ctfoff;		/* CTF offset table */
	caddr_t		typoff;		/* Type offset table */
	long		typlen;		/* Number of type entries. */
	Elf_Addr	pcpu_start;	/* Pre-relocation pcpu set start. */
	Elf_Addr	pcpu_stop;	/* Pre-relocation pcpu set stop. */
	Elf_Addr	pcpu_base;	/* Relocated pcpu set address. */
#ifdef VIMAGE
	Elf_Addr	vnet_start;	/* Pre-relocation vnet set start. */
	Elf_Addr	vnet_stop;	/* Pre-relocation vnet set stop. */
	Elf_Addr	vnet_base;	/* Relocated vnet set address. */
#endif
#ifdef GDB
	struct link_map	gdb;		/* hooks for gdb */
#endif
} *elf_file_t;


static void get_elf(struct module *module){
	elf_file_t file = (elf_file_t)module_file(module);
	
	objc_log("ELF file dump (%p):\n", file);
	objc_log("\preloaded: \t\t%i\n", file->preloaded);
	objc_log("\address: \t\t%p\n", file->address);
	objc_log("\ddbsymtab: \t\t0x%lx\n", file->ddbsymtab);


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


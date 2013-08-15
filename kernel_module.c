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
#include <sys/vnode.h>
#include <sys/malloc.h>

SET_DECLARE(objc_module_list_set, struct objc_loader_module);

// extern void run_tests(void);

MALLOC_DECLARE(M_LIBUNWIND_FAKE);
MALLOC_DEFINE(M_LIBUNWIND_FAKE, "fake", "fake");

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

static void list_sections(caddr_t firstpage){
	
	// (ef->strtab + ref->st_name)
	int error = 0;
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
	
	
	Elf_Phdr *phdr = (Elf_Phdr *) (firstpage + ehdr->e_phoff);
	Elf_Phdr *phlimit = phdr + ehdr->e_phnum;
	int nsegs = 0;
	Elf_Phdr *phdyn = NULL;
	Elf_Phdr *phphdr = NULL;
	const int MAXSEGS = 32;
	Elf_Phdr *segs[MAXSEGS];
	while (phdr < phlimit) {
		switch (phdr->p_type) {
			case PT_LOAD:
				if (nsegs == MAXSEGS) {
					objc_log("Too many segments!\n");
					return;
				}
				
				segs[nsegs] = phdr;
				++nsegs;
				break;
			case PT_PHDR:
				phphdr = phdr;
				break;
			case PT_DYNAMIC:
				phdyn = phdr;
				break;
			case PT_INTERP:
				error = ENOSYS;
				return;
		}
		
		objc_log("PHDR dump:\n");
		objc_log("\tp_type: \t\t%u\n", phdr->p_type);
		objc_log("\tp_flags: \t\t%u\n", phdr->p_flags);
		objc_log("\tp_offset: \t\t0x%lx\n", phdr->p_offset);
		objc_log("\tp_vaddr: \t\t0x%lx\n", phdr->p_vaddr);
		objc_log("\tp_paddr: \t\t0x%lx\n", phdr->p_paddr);
		objc_log("\tp_filesz: \t\t%lu\n", phdr->p_filesz);
		objc_log("\tp_memsz: \t\t%lu\n", phdr->p_memsz);
		objc_log("\tp_align: \t\t%lu\n", phdr->p_align);
		objc_log("===================\n");
		
		++phdr;
	}
	
	
	Elf_Shdr *shdr = (Elf_Shdr *) (firstpage + ehdr->e_shoff);
	Elf_Shdr *shlimit = shdr + ehdr->e_shnum;
	while (shdr < shlimit) {
		objc_log("SHDR dump:\n");
		objc_log("\tsh_name: \t\t%lx\n", (unsigned long)shdr->sh_name);
		objc_log("\tsh_type: \t\t%lx\n", (unsigned long)shdr->sh_type);
		objc_log("\tsh_flags: \t\t0x%lx\n", (unsigned long)shdr->sh_flags);
		objc_log("\tsh_addr: \t\t0x%lx\n", (unsigned long)shdr->sh_addr);
		objc_log("\tsh_offset: \t\t0x%lx\n", (unsigned long)shdr->sh_offset);
		objc_log("\tsh_size: \t\t%lu\n", (unsigned long)shdr->sh_size);
		objc_log("\tsh_link: \t\t%lx\n", (unsigned long)shdr->sh_link);
		objc_log("\tsh_info: \t\t%lx\n", (unsigned long)shdr->sh_info);
		objc_log("\tsh_entsize: \t\t%lu\n", (unsigned long)shdr->sh_entsize);
		objc_log("===================\n");
		
		++shdr;
	}
}


static void get_elf(struct module *module){
	linker_file_t file = module_file(module);
	elf_file_t elf = (elf_file_t)file;
	int flags;
	int error = 0;
	ssize_t resid;
	
	int readsize = 250000;

	struct nameidata nd;
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, file->pathname, curthread);
	flags = FREAD;
	error = vn_open(&nd, &flags, 0, NULL);
	if (error != 0) {
		objc_log("Failed to open file (%s)!\n", file->pathname);
		return;
	}
	
	NDFREE(&nd, NDF_ONLY_PNBUF);
	if (nd.ni_vp->v_type != VREG) {
		objc_log("Wrong v_type (%s)!\n", file->pathname);
		return;
	}
	
	caddr_t firstpage = malloc(readsize, M_LIBUNWIND_FAKE, M_WAITOK);
	error = vn_rdwr(UIO_READ, nd.ni_vp, firstpage, readsize, 0,
					UIO_SYSSPACE, IO_NODELOCKED, curthread->td_ucred, NOCRED,
					&resid, curthread);

	objc_log("strtab %p count: %i\n", elf->strtab, elf->strsz);;
	
	objc_log("ELF file dump:\n");
	objc_log("\t address: \t\t%p\n", elf->address);
	objc_log("\t dynamic: \t\t%p\n", elf->dynamic);
	objc_log("\t strtab: \t\t%p\n", elf->strtab);
	objc_log("\t strsize: \t\t%lu\n", (unsigned long)elf->strsz);
	objc_log("\t ddbstrtab: \t\t%p\n", elf->ddbstrtab);
	objc_log("\tsh_size: \t\t%lu\n", (unsigned long)elf->ddbstrcnt);
	
	
	VOP_UNLOCK(nd.ni_vp, 0);
	vn_close(nd.ni_vp, FREAD, curthread->td_ucred, curthread);
	return;
	
	list_sections(firstpage);
	
	
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


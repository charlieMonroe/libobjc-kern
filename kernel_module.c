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

#include <sys/elf.h>
#include <sys/namei.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/vnode.h>

#include <machine/elf.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_extern.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>

#include <sys/link_elf.h>

typedef struct {
	void		*addr;
	Elf_Off		size;
	int		flags;
	int		sec;	/* Original section */
	char		*name;
} Elf_progent;

typedef struct {
	Elf_Rel		*rel;
	int		nrel;
	int		sec;
} Elf_relent;

typedef struct {
	Elf_Rela	*rela;
	int		nrela;
	int		sec;
} Elf_relaent;


SET_DECLARE(objc_module_list_set, struct objc_loader_module);

extern void run_tests(void);

MALLOC_DECLARE(M_LIBUNWIND_FAKE);
MALLOC_DEFINE(M_LIBUNWIND_FAKE, "fake", "fake");

typedef struct elf_file {
	struct linker_file lf;		/* Common fields */
  
	int		preloaded;
	caddr_t		address;	/* Relocation address */
	vm_object_t	object;		/* VM object to hold file pages */
	Elf_Shdr	*e_shdr;
  
	Elf_progent	*progtab;
	int		nprogtab;
  
	Elf_relaent	*relatab;
	int		nrelatab;
  
	Elf_relent	*reltab;
	int		nreltab;
  
	Elf_Sym		*ddbsymtab;	/* The symbol table we are using */
	long		ddbsymcnt;	/* Number of symbols */
	caddr_t		ddbstrtab;	/* String table */
	long		ddbstrcnt;	/* number of bytes in string table */
  
	caddr_t		shstrtab;	/* Section name string table */
	long		shstrcnt;	/* number of bytes in string table */
  
	caddr_t		ctftab;		/* CTF table */
	long		ctfcnt;		/* number of bytes in CTF table */
	caddr_t		ctfoff;		/* CTF offset table */
	caddr_t		typoff;		/* Type offset table */
	long		typlen;		/* Number of type entries. */
  
} *elf_file_t;

static void list_sections(caddr_t firstpage){
	
	// (ef->strtab + ref->st_name)
	Elf_Ehdr *ehdr = (Elf_Ehdr *)firstpage;
	Elf_Shdr *shdr;
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
	
	
	int offset = (ehdr->e_shstrndx * ehdr->e_shentsize) + ehdr->e_shoff;
	Elf_Shdr *sh_strtab = (Elf_Shdr*)(firstpage + offset);
	objc_log("SH_STRTAB dump:\n");
	objc_log("\tsh_name: \t\t%lx\n", (unsigned long)sh_strtab->sh_name);
	objc_log("\tsh_type: \t\t%lx vs %lx\n", (unsigned long)sh_strtab->sh_type, (unsigned long)SHT_STRTAB);
	objc_log("\tsh_flags: \t\t0x%lx\n", (unsigned long)sh_strtab->sh_flags);
	objc_log("\tsh_addr: \t\t0x%lx\n", (unsigned long)sh_strtab->sh_addr);
	objc_log("\tsh_offset: \t\t0x%lx\n", (unsigned long)sh_strtab->sh_offset);
	objc_log("\tsh_size: \t\t%lu\n", (unsigned long)sh_strtab->sh_size);
	objc_log("\tsh_link: \t\t%lx\n", (unsigned long)sh_strtab->sh_link);
	objc_log("\tsh_info: \t\t%lx\n", (unsigned long)sh_strtab->sh_info);
	objc_log("\tsh_entsize: \t\t%lu\n", (unsigned long)sh_strtab->sh_entsize);
	
	const char *const sh_strtab_p = firstpage + sh_strtab->sh_offset;
	
	shdr = (Elf_Shdr *) (firstpage + ehdr->e_shoff);
	Elf_Shdr *shlimit = shdr + ehdr->e_shnum;
	while (shdr < shlimit) {
		objc_log("SHDR dump:\n");
		objc_log("\tsh_name: \t\t%lx -> %s\n", (unsigned long)shdr->sh_name, sh_strtab_p + shdr->sh_name);
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
  
  elf_file_t efile = (elf_file_t)module_file(module);
	
	objc_log("ELF file dump (%p):\n", efile);
	objc_log("\tpreloaded: \t\t%i\n", efile->preloaded);
	objc_log("\taddress: \t\t%p\n", efile->address);
	objc_log("\tshdr: \t\t%p\n", efile->e_shdr);
  
  Elf_Shdr *eh_frame_shdr = NULL;
  Elf_Shdr *shdr = efile->e_shdr;
  for (int i = 0; i < 56; ++i) {
    const char *name = efile->shstrtab + shdr->sh_name;
    if (objc_strings_equal(name, "set_objc_module_list_set")){
      eh_frame_shdr = shdr;
    }
    
		objc_log("SHDR dump:\n");
		objc_log("\tsh_name: \t\t%lx -> %s\n", (unsigned long)shdr->sh_name, name);
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
  
  if (eh_frame_shdr != NULL){
    struct objc_loader_module **module_begin = SET_BEGIN(objc_module_list_set);
    objc_log("Found .eh_frame, it's at offset 0x%lx\n", (unsigned long)eh_frame_shdr->sh_offset);
    objc_log("\t addr \t\t %p\n", efile->address);
    objc_log("\t addr + off \t\t %p\n", efile->address + eh_frame_shdr->sh_offset);
    objc_log("\t *module_begin \t\t %p\n", *module_begin);
    objc_log("\t module_begin \t\t %p\n", module_begin);
  }
  
  return;
  
  
	linker_file_t file = module_file(module);
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

	list_sections(firstpage);
	
	VOP_UNLOCK(nd.ni_vp, 0);
	vn_close(nd.ni_vp, FREAD, curthread->td_ucred, curthread);
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
		/*_objc_load_modules(SET_BEGIN(objc_module_list_set),
						   SET_LIMIT(objc_module_list_set));

		run_tests();*/
		
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


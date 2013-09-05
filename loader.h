
#ifndef OBJC_PROTOTYPES_H
#define OBJC_PROTOTYPES_H

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

/* Defined ABI versions. */
enum objc_abi_version {
	objc_abi_version_kernel_1 = 0x301
};

struct objc_symbol_table {
	/* Number of selector references in the 'selector_references' field. */
	unsigned int                      selector_reference_count;
	
	/* An array of selector references. */
	struct objc_selector_reference    *selector_references;
	
	/* Number of classes. */
	unsigned short                    class_count;
	
	/* Actual class list. */
	struct objc_class                 **classes;
	
	/* Number of categories. */
	unsigned short					  category_count;
	
	/* Actual list of categories. */
	struct objc_category			  **categories;
	
	/* Number of protocols. */
	unsigned short					  protocol_count;
	
	/* Number of protocols. */
	struct objc_protocol			  **protocols;
};

struct objc_loader_module {
	/* Name of the module. */
	const char *name;
	
	/* Pointer to the symbol table structure. */
	struct objc_symbol_table *symbol_table;
	
	/* Version of the module. */
	int version;
};



PRIVATE void _objc_load_module(struct objc_loader_module *module,
							   void *kernel_module);
PRIVATE void _objc_load_modules(struct objc_loader_module **begin,
                                struct objc_loader_module **end,
								void *kernel_module);


PRIVATE BOOL _objc_unload_modules(struct objc_loader_module **begin,
								  struct objc_loader_module **end,
								  void *kernel_module);


#endif

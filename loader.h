
#ifndef OBJC_PROTOTYPES_H
#define OBJC_PROTOTYPES_H

#include "types.h"
#include "selector.h"

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
	unsigned short					category_count;
	
  /* Actual list of categories. */
  struct objc_category    **categories;
};

struct objc_loader_module {
	/* Name of the module. */
	const char *name;
	
	/* Pointer to the symbol table structure. */
	struct objc_symbol_table *symbol_table;
	
	/* Version of the module. */
	int version;
};



PRIVATE void _objc_load_module(struct objc_loader_module *module);


#endif


#ifndef OBJC_PROTOTYPES_H
#define OBJC_PROTOTYPES_H

typedef enum {
	objc_abi_version_kernel_1 = (unsigned long)'ker1'
} objc_abi_version;


typedef struct {
	unsigned long selector_count;
	struct objc_selector *selectors;
	
	unsigned short class_count;
	unsigned short category_count;
	
	void *definitions[];
} objc_symbol_table;

typedef struct {
	objc_abi_version version;
	unsigned long size;
	const char *name;
	objc_symbol_table *symbol_table;
} objc_loader_module;


#endif

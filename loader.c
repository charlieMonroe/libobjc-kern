#include "os.h"
#include "kernobjc/types.h"
#include "kernobjc/protocol.h"
#include "types.h"
#include "loader.h"
#include "selector.h"
#include "init.h"
#include "class_registry.h"
#include "private.h"

PRIVATE void _objc_load_module(struct objc_loader_module *module){
	if (!objc_runtime_initialized){
		objc_runtime_init();
	}
	
	/* First, check the version. */
	objc_assert(module->version == objc_abi_version_kernel_1,
				"Unknown version of module version (%i)\n", module->version);
	
	objc_debug_log("Loading a module named %s\n", module->name);
	
	struct objc_symbol_table *table = module->symbol_table;
	objc_debug_log("Symbol table: %p\n", table);
	objc_debug_log("\tSelector ref count: %i\n", table->selector_reference_count);
	objc_debug_log("\tSelector refs: %p\n", table->selector_references);
	objc_debug_log("\tClass count: %i\n", table->class_count);
	objc_debug_log("\tClasses: %p\n", table->classes);
	objc_debug_log("\tCategory count: %i\n", table->category_count);
	objc_debug_log("\tCategories: %p\n", table->categories);
	objc_debug_log("\tProtocol count: %i\n", table->protocol_count);
	objc_debug_log("\tProtocols: %p\n", table->protocols);
	
	/* Register selectors. */
	objc_register_selector_array(table->selector_references,
								 table->selector_reference_count);
	
#if OBJC_DEBUG_LOG
	for (int i = 0; i < table->class_count; ++i){
		objc_debug_log("Should be registering class[%i; %p] %s\n", i,
					   table->classes[i],
					   table->classes[i]->name);
		
		Class super_class = table->classes[i]->super_class;
		objc_debug_log("\tSuperclass[%p]: %s\n",
					   super_class,
					   super_class == NULL ? "" : super_class->name);
	}
#endif
	
	objc_class_register_classes(table->classes, table->class_count);
	
	
	for (int i = 0; i < table->category_count; ++i){
		objc_debug_log("Should be registering category[%i; %p] %s for class %s\n", i,
					   table->categories[i],
					   table->categories[i]->category_name,
					   table->categories[i]->class_name);
		objc_category_try_load(table->categories[i]);
	}
	
	for (int i = 0; i < table->protocol_count; ++i){
		objc_debug_log("Registering protocol %s\n", table->protocols[i]->name);
		objc_registerProtocol(table->protocols[i]);
	}
}


PRIVATE void _objc_load_modules(struct objc_loader_module **begin,
                                struct objc_loader_module **end)
{
	objc_debug_log("Should be loading modules from array bounded by [%p, %p)\n",
				   begin, end);
	objc_debug_log("Module count: %i\n", (int)(end - begin));
	
	struct objc_loader_module **module_ptr;
	int counter = 0;
	for (module_ptr = begin; module_ptr < end; module_ptr++) {
		objc_debug_log("\t[%i] %p -> %p\n", counter, module_ptr, *module_ptr);
		_objc_load_module(*module_ptr);
	}
}

#ifdef _KERNEL
PRIVATE BOOL
_objc_load_kernel_module(struct module *module)
{
	objc_assert(module != NULL, "Cannot load a NULL module!\n");
	
	struct linker_file *file = module_file(module);
	struct objc_loader_module **begin = NULL;
	struct objc_loader_module **end = NULL;
	int count = 0;
	
	linker_file_lookup_set(file, "objc_module_list_set", &begin, &end, &count);
	
	if (count == 0){
		objc_debug_log("Couldn't find any ObjC data for module %s!\n",
					   module_getname(module));
		return NO;
	}
	
	_objc_load_modules(begin, end);
	return YES;
}
#endif

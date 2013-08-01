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
  objc_debug_log("\tSelector class count: %i\n", table->class_count);
  objc_debug_log("\tSelector classes: %p\n", table->classes);
  objc_debug_log("\tSelector category count: %i\n", table->category_count);
	
	/* Register selectors. */
	objc_register_selector_array(table->selector_references,
                               table->selector_reference_count);
	
	
  for (int i = 0; i < table->class_count; ++i){
    objc_debug_log("Should be registering class[%i; %p] %s\n", i,
                   table->classes[i],
                   table->classes[i]->name);
    
    Class super_class = table->classes[i]->super_class;
    objc_debug_log("\tSuperclass[%p]: %s\n",
                   super_class,
                   super_class == NULL ? "" : super_class->name);
  }
  
  objc_class_register_classes(table->classes, table->class_count);
	
  
  for (int i = 0; i < table->category_count; ++i){
    objc_debug_log("Should be registering category[%i; %p] %s for class %s\n", i,
                   table->categories[i],
                   table->categories[i]->category_name,
                   table->categories[i]->class_name);
    objc_category_try_load(table->categories[i]);
  }
  
	/* TODO: Register categories. */
	
}


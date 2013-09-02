#include "os.h"
#include "kernobjc/runtime.h"
#include "types.h"
#include "sarray2.h"
#include "dtable.h"
#include "runtime.h"
#include "loader.h"
#include "selector.h"
#include "init.h"
#include "class_registry.h"
#include "private.h"


#ifndef _KERNEL
static inline const char *module_getname(void *module){
	return "USERLAND";
}
static inline void *module_get_start(void *module){
	return (void*)-1;
}
static inline size_t module_get_size(void *module){
	return 0;
}
static inline BOOL module_contains_IMP(void *module, void *IMP){
	return (IMP >= module_get_start(module))
			&& (IMP < module_get_start(module) + module_get_size(module));
}
#else
static inline BOOL module_contains_IMP(void *module, void *IMP){
	struct module *kernel_module = (struct module*)module;
	struct linker_file *file = module_file(kernel_module);
	return ((char*)IMP >= file->address)
			&& ((char*)IMP < file->address + file->size);
}
#endif

/* Default unloaded module method. */
static void
__objc_unloaded_module_implementation_called(id sender, SEL _cmd)
{
	objc_msgSend(objc_getClass("__KKUnloadedModuleException"),
				 sel_getNamed("raiseUnloadedModuleException:selector:"),
				 sender,
				 _cmd);
}

/* The hook. */
IMP objc_unloaded_module_method = (IMP)__objc_unloaded_module_implementation_called;

static void
_objc_unload_IMPs_in_class(Class cl, void *kernel_module){
	if (cl->methods == NULL){
		return;
	}
	
	for (objc_method_list *list = cl->methods; list != NULL; list = list->next){
		for (int i = 0; i < list->size; ++i){
			Method m = &list->list[i];
			if (module_contains_IMP(kernel_module, m->implementation)){
				IMP old_imp = m->implementation;
				m->implementation = objc_unloaded_module_method;
				
				/* Since the selector name and types can actually be also 
				 * allocated in the module, we better update the strings
				 * as well.
				 */
				m->selector_name = "__objc_unloaded_method";
				m->selector_types = sizeof(void*) == 4 ? "v8@0:4" : "v16@0:8";
				
				/* Update the dtable! */
				SparseArray *arr = (SparseArray*)cl->dtable;
				if (arr != NULL && arr != uninstalled_dtable){
					struct objc_slot *slot = SparseArrayLookup(arr, m->selector);
					if (slot != NULL && slot->implementation == old_imp){
						slot->implementation = m->implementation;
						
						slot->types = m->selector_types;
						
						++slot->version;
					}
				}
			}
		}
	}
}

static void
_objc_unload_IMPs_in_branch(Class branch, void *kernel_module)
{
	if (branch == Nil){
		return;
	}
	
	for (Class c = branch->subclass_list; c != Nil; c = c->sibling_list){
		_objc_unload_IMPs_in_branch(c, kernel_module);
	}
	
	_objc_unload_IMPs_in_class(branch, kernel_module);
	_objc_unload_IMPs_in_class(branch->isa, kernel_module);
}

static void
_objc_unload_IMPs_from_kernel_module(void *kernel_module)
{
	Class root = objc_class_get_root_class_list();
	for (Class c = root; c != Nil; c = c->sibling_list){
		_objc_unload_IMPs_in_branch(c, kernel_module);
	}
}

static void
_objc_unload_classes_in_branch(Class branch, void *kernel_module)
{
	if (branch == Nil){
		return;
	}
	
	for (Class c = branch->subclass_list; c != Nil; c = c->sibling_list){
		_objc_unload_classes_in_branch(c, kernel_module);
	}
	
	objc_debug_log("\t Checking class %s - module %p, sibling %p (%s)\n",
				   class_getName(branch), branch->kernel_module,
				   branch->sibling_list, branch->sibling_list == Nil ? "(null)" :
				   class_getName(branch->sibling_list));
	if (branch->kernel_module == kernel_module){
		/* Remove this node. */
		objc_unload_class(branch);
	}
}

static void
_objc_unload_classes_in_kernel_module(void *kernel_module)
{
	Class root = objc_class_get_root_class_list();
	
	objc_debug_log("\t Unloading classes in module %p, root class %s\n",
				   kernel_module, root == Nil ? "(null)" : class_getName(root));
	
	for (Class c = root; c != Nil; c = c->sibling_list){
		_objc_unload_classes_in_branch(c, kernel_module);
	}
}

static BOOL
_objc_can_unload_class(Class cl, void *kernel_module)
{
	for (Class c = cl; c != Nil; c = c->sibling_list){
		if (c->kernel_module != kernel_module){
			objc_log("Cannot unload class %s since it is declared in module %s"
					 " and is a subclass of a class declared in this module.\n",
					 class_getName(c), module_getname(kernel_module));
			return NO;
		}
		
		if (c->subclass_list != Nil
			&& !_objc_can_unload_class(c->subclass_list, kernel_module)){
			return NO;
		}
	}
	return YES;
}

static BOOL
_objc_can_unload_protocol(Protocol *p, Class root_class, void *kernel_module)
{
	for (Class c = root_class; c != Nil; c = c->sibling_list){
		if (c->kernel_module == kernel_module){
			/* 
			 * We can safely assume that if the class is from this module,
			 * it doesn't matter if the protocol is adopted on this class
			 * or its subclasses since we've already run the class check.
			 */
			continue;
		}
		
		objc_protocol_list *protocol_list = c->protocols;
		if (protocol_list != NULL){
			while (protocol_list != NULL) {
				for (int i = 0; i < protocol_list->size; ++i){
					if (protocol_list->list[i] == p){
						objc_log("Cannot unload protocol %s since it is used in"
								 " class %s and this class is declared in module"
								 " %s which is still loaded.\n",
								 p->name,
								 class_getName(c),
								 c->kernel_module == NULL ? "(null)" :
								 module_getname(c->kernel_module));
						return NO;
					}
				}
				protocol_list = protocol_list->next;
			}
		}
		
		if (c->subclass_list != Nil){
			if (!_objc_can_unload_protocol(p, c->subclass_list, kernel_module)){
				return NO;
			}
		}
	}
	
	return YES;
}

static inline BOOL
_objc_module_check_dependencies_for_unloading(struct objc_loader_module *module,
											  void *kernel_module)
{
	/* First, check classes */
	for (int i = 0; i < module->symbol_table->class_count; ++i){
		Class cl = module->symbol_table->classes[i];
		if (cl->subclass_list == Nil){
			continue;
		}
		
		if (!_objc_can_unload_class(cl->subclass_list, kernel_module)){
			return NO;
		}
	}
	
	/* 
	 * Now try protocols. These are a little bit harder. We need to go through
	 * all the classes and see if any of the classes adopts the protocol since
	 * protocol adoption can be done at run-time.
	 */
	Class root = objc_class_get_root_class_list();
	for (int i = 0; i < module->symbol_table->protocol_count; ++i){
		Protocol *p = module->symbol_table->protocols[i];
		if (!_objc_can_unload_protocol(p, root, kernel_module)){
			return NO;
		}
	}
	
	return YES;
}


PRIVATE void
_objc_load_module(struct objc_loader_module *module,
							   void *kernel_module)
{
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
	
	for (int i = 0; i < table->class_count; ++i){
		/* Mark the class as owned by the kernel module. */
		table->classes[i]->kernel_module = kernel_module;
		
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
	
	for (int i = 0; i < table->protocol_count; ++i){
		objc_debug_log("Registering protocol %s\n", table->protocols[i]->name);
		objc_registerProtocol(table->protocols[i]);
	}
}


PRIVATE void _objc_load_modules(struct objc_loader_module **begin,
                                struct objc_loader_module **end,
								void *kernel_module)
{
	objc_debug_log("Should be loading modules from array bounded by [%p, %p)\n",
				   begin, end);
	objc_debug_log("Module count: %i\n", (int)(end - begin));
	
	struct objc_loader_module **module_ptr;
	for (module_ptr = begin; module_ptr < end; module_ptr++) {
		_objc_load_module(*module_ptr, kernel_module);
	}
	
	/* Now try to resolve all classes. */
	objc_class_resolve_links();
}

PRIVATE BOOL
_objc_unload_modules(struct objc_loader_module **begin,
								  struct objc_loader_module **end,
								  void *kernel_module)
{
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	struct objc_loader_module **module_ptr;
	for (module_ptr = begin; module_ptr < end; module_ptr++) {
		objc_debug_log("Checking module dependencies for %s\n",
					   (*module_ptr)->name);
		if (!_objc_module_check_dependencies_for_unloading(*module_ptr,
														   kernel_module)){
			objc_debug_log("Failed checking module dependencies for %s\n",
						   (*module_ptr)->name);
			return NO;
		}
	}
	
	objc_debug_log("No dependencies, unloading classes...\n");
	
	/* 
	 * At this point, we can be certain that it is safe to unload all the
	 * classes and protocols.
	 */
	_objc_unload_classes_in_kernel_module(kernel_module);
	
	objc_debug_log("Unloading protocols...\n");
	
	for (module_ptr = begin; module_ptr < end; module_ptr++) {
		struct objc_loader_module *module = *module_ptr;
		for (int i = 0; i < module->symbol_table->protocol_count; ++i){
			objc_protocol_unload(module->symbol_table->protocols[i]);
		}
	}
	
	
	objc_debug_log("Unloading IMPs...\n");
	
	/*
	 * Now for the fun part. Go through all the IMPs...
	 */
	_objc_unload_IMPs_from_kernel_module(kernel_module);
	
	objc_debug_log("All done unloading module %s.\n", module_getname(kernel_module));
	
	return YES;
}

#ifdef _KERNEL
PRIVATE BOOL
_objc_load_kernel_module(struct module *kernel_module)
{
	objc_assert(kernel_module != NULL, "Cannot load a NULL module!\n");
	
	struct linker_file *file = module_file(kernel_module);
	struct objc_loader_module **begin = NULL;
	struct objc_loader_module **end = NULL;
	int count = 0;
	
	linker_file_lookup_set(file, "objc_module_list_set", &begin, &end, &count);
	
	if (count == 0){
		objc_debug_log("Couldn't find any ObjC data for module %s!\n",
					   module_getname(kernel_module));
		return NO;
	}
	
	_objc_load_modules(begin, end, kernel_module);
	return YES;
}

PRIVATE BOOL
_objc_unload_kernel_module(struct module *kernel_module){
	objc_assert(kernel_module != NULL, "Cannot unload a NULL module!\n");
	
	/* Locking module lock. */
	MOD_SLOCK;
	
	objc_debug_log("Unloading module %s\n", module_getname(kernel_module));
	
	struct linker_file *file = module_file(kernel_module);
	struct objc_loader_module **begin = NULL;
	struct objc_loader_module **end = NULL;
	int count = 0;
	
	linker_file_lookup_set(file, "objc_module_list_set", &begin, &end, &count);
	
	if (count == 0){
		objc_debug_log("Couldn't find any ObjC data for module %s, nothing to "
					   "unload\n", module_getname(kernel_module));
		MOD_SUNLOCK;
		return YES;
	}
	
	BOOL result = _objc_unload_modules(begin, end, kernel_module);
	MOD_SUNLOCK;
	return result;
}


#endif



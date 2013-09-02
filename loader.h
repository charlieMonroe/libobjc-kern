
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

#ifdef _KERNEL
/*
 * Finds the ObjC modules for the kernel module and loads them. Returns YES
 * if anything was loaded.
 */
PRIVATE BOOL _objc_load_kernel_module(struct module *kernel_module);

/*
 * This is the fun part. Since the runtime doesn't copy any class structures,
 * when you unload a module and there are still existing subclasses or classes
 * declared in such a module, the whole thing is likely going to crash sooner or
 * later (most definitely when unloading the kernel module with this runtime.
 *
 * Copying the class structures isn't going to solve anything since most likely
 * the module declared some methods as well and the IMPs after unloading point
 * somewhere into the memory where the module once was, but is now gone and the
 * pages were most likely unmapped since, resulting in a kernel panic, or - if
 * they were since mapped to someone else, it is most likely to corrupt some 
 * data.
 *
 * When unloading a module, the kernel runtime simply goes through the classes 
 * declared in the module and sees whether any of these classes has any subclass
 * that is *not* declared by this module, which includes user-allocated (!) 
 * classes. If there are such subclasses, this function returns NO and you MUST
 * NOT allow the module to be unloaded!
 *
 * The same pretty much goes for protocols as well. Selectors are simply left,
 * since there's no way of knowing which modules use which selectors and keeping
 * track of this is probably not worth the trouble.
 *
 * After this, the runtime goes into the trouble of iterating over all classes
 * and for each class over all methods declared and sees if any of the IMPs 
 * points into the memory mapped by this kernel module you are unloading. This
 * is because your module might have installed some methods on other classes
 * using categories. 
 *
 * If such an IMP pointer is found, the runtime does not remove the method as
 * that would pretty much mean a lot of reallocations of the method lists, but
 * instead replaces the IMP pointer by a function that raises an ObjC exception
 * which you can actually catch and deal with it. Note, however, that this 
 * exception isn't a NSException, but a special subclass of KKObject declared
 * in the kernel. See KKObjects.h.
 *
 * You can change this IMP pointer replacement using the objc_unloaded_module_method
 * hook.
 *
 */
PRIVATE BOOL _objc_unload_kernel_module(struct module *kernel_module);

#endif

#endif

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "selector.h" /* For sel_registerName. */
#include "kernobjc/runtime.h"
#include "sarray2.h"
#include "dtable.h"
#include "class.h"
#include "private.h"
#include "runtime.h"
#include "utils.h"

/*
 * Adds methods from the array 'm' into the method_list. The m array doesn't
 * have to be NULL-terminated, and has to contain 'count' methods.
 *
 * The m array is copied over to be NULL-terminated, but the Method 'objects'
 * are not.
 */
static inline void
_add_methods_to_class(Class cl, Method *m, unsigned int count)
{
	objc_method_list *list = objc_method_list_create(count);
	for (int i = 0; i < count; ++i){
		list->list[i] = *m[i];
	}
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	if (cl->methods == NULL){
		cl->methods = list;
	}else{
		cl->methods = objc_method_list_append(cl->methods, list);
	}
	
	if (cl->dtable != uninstalled_dtable){
		dtable_add_method_list_to_class(cl, list);
	}
}

/*
 * Recursively adds method list to target.
 */
static inline void _class_addMethodsFromList(Class target,
											 objc_method_list *source_list)
{
	if (source_list->next != NULL){
		_class_addMethodsFromList(target, source_list->next);
	}
	
	unsigned int count = source_list->size;
	objc_method_list *list = objc_method_list_create(count);
	for (int i = 0; i < count; ++i){
		list->list[i] = source_list->list[i];
	}
	
	list->next = target->methods;
	target->methods = list;
	
	if (target->dtable != uninstalled_dtable){
		dtable_add_method_list_to_class(target, list);
	}
}

/*
 * Adds instance methods to class cl.
 */
static inline void
_add_methods(Class cl, Method *m, unsigned int count)
{
	if (cl == NULL || m == NULL){
		return;
	}
	
	_add_methods_to_class(cl, m, count);
}


#pragma mark -
#pragma mark Method creation

/* Public functions are documented in the header file. */

static Method
objc_method_create(SEL selector, IMP implementation)
{
	objc_assert(implementation != NULL, "Trying to create a method with "
		    "NULL implementation!");
	
	Method m = objc_alloc(sizeof(struct objc_method), M_METHOD_TYPE);
	m->selector = selector;
	
	/*
	 * Even though the selector_name and selector_types
	 * fields are used only during loading, there's really
	 * no reason not to include them here anyway since
	 * they are just pointers to the actual Selector structure
	 * anyway.
	 */
	m->selector_name = sel_getName(selector);
	m->selector_types = sel_getTypes(selector);
	
	m->implementation = implementation;
	m->version = 0;
	return m;
}

IMP
method_getImplementation(Method method)
{
	if (method == NULL){
		return NULL;
	}
	return method->implementation;
}

SEL
method_getName(Method m)
{
	return m == NULL ? null_selector : m->selector;
}

const char *
method_getTypeEncoding(Method m)
{
	if (m == NULL){
		return NULL;
	}
	return sel_getTypes(m->selector);
}

IMP
method_setImplementation(Method m, IMP imp)
{
	if (m == NULL){
		return NULL;
	}
	
	IMP old_impl = m->implementation;
	m->implementation = imp;
	
	objc_updateDtableForClassContainingMethod(m);
	
	return old_impl;
}

#pragma mark -
#pragma mark Adding methods

void
class_addMethodsFromClass(Class target, Class source)
{
	if (target == Nil || source == Nil){
		return;
	}
	
	if (source->methods == NULL || source->methods->size == 0){
		return;
	}
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	objc_method_list *source_list = source->methods;
	_class_addMethodsFromList(target, source_list);
}

BOOL
class_addMethod(Class cls, SEL selector, IMP imp, const char *types)
{
	if (cls == NULL || selector == null_selector || imp == NULL){
		objc_debug_log("Not adding method to class %p as either IMP %p is NULL,"
					   " or selector is null_selector [%d]\n", cls, imp, selector);
		return NO;
	}
	
	objc_assert(objc_strings_equal(sel_getTypes(selector), types),
				"Registering method with incompatible types!\n");
	
	Method m = objc_method_create(selector, imp);
	_add_methods(cls, &m, 1);
	objc_dealloc(m, M_METHOD_TYPE);
	return YES;
}


void
method_exchangeImplementations(Method m1, Method m2)
{
	if (m1 == NULL || m2 == NULL){
		return;
	}
	IMP imp = m2->implementation;
	m2->implementation = m1->implementation;
	m1->implementation = imp;
	
	objc_updateDtableForClassContainingMethod(m1);
	objc_updateDtableForClassContainingMethod(m2);
}


#pragma mark -
#pragma mark Replacing methods

IMP
class_replaceMethod(Class cls, SEL name, IMP imp)
{
	if (cls == Nil || name == null_selector || imp == NULL){
		return NULL;
	}
	
	Method m = class_getMethod(cls, name);
	if (m == NULL){
		Method new_method = objc_method_create(name, imp);
		_add_methods(cls, &new_method, 1);
		objc_dealloc(new_method, M_METHOD_TYPE);
	}else{
		m->implementation = imp;
		++m->version;
	}
	
	if (cls->flags.resolved){
		objc_update_dtable_for_class(cls);
	}
	
	return m == NULL ? NULL : m->implementation;
}

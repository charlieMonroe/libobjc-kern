#include "method.h"
#include "selector.h" /* For sel_registerName. */
#include "os.h" /* For objc_alloc. */
#include "utils.h" /* For objc_strcpy */
#include "dtable.h"
#include "class.h"


/**
 * Adds methods from the array 'm' into the method_list. The m array doesn't
 * have to be NULL-terminated, and has to contain 'count' methods.
 *
 * The m array is copied over to be NULL-terminated, but the Method 'objects'
 * are not.
 */
OBJC_INLINE void _add_methods_to_class(Class cl, Method *m, unsigned int count){
	
	// TODO locking
	objc_method_list *list = objc_method_list_create(count);
	for (int i = 0; i < count; ++i){
		list->method_list[i] = *m[i];
	}
	
	cl->methods = objc_method_list_append(cl->methods, list);
	dtable_add_method_list_to_class(cl, cl->methods);
}


/**
 * Adds instance methods to class cl.
 */
OBJC_INLINE void _add_methods(Class cl, Method *m, unsigned int count){
	if (cl == NULL || m == NULL){
		return;
	}
	
	_add_methods_to_class(cl, m, count);
}


#pragma mark -
#pragma mark Method creation

/* Public functions are documented in the header file. */

Method objc_method_create(SEL selector, IMP implementation){
	objc_assert(implementation != NULL, "Trying to create a method with NULL implementation!");
	
	Method m = objc_alloc(sizeof(struct objc_method));
	m->selector = selector;
	
	/**
	 * Even though the selector_name and selector_types
	 * fields are used only during loading, there's really
	 * no reason not to include them here anyway since
	 * they are just pointers to the actual Selector structure
	 * anyway.
	 */
	m->selector_name = sel_getName(selector);
	m->selector_types = objc_selector_get_types(selector);
	
	m->implementation = implementation;
	m->version = 0;
	return m;
}

IMP objc_method_get_implementation(Method method){
	objc_assert(method != NULL, "Getting implementation of NULL method!");
	return method->implementation;
}

#pragma mark -
#pragma mark Adding methods


BOOL class_addMethod(Class cls, SEL selector, IMP imp){
	if (cls == NULL || selector == 0 || imp == NULL){
		return NO;
	}
	
	Method m = objc_method_create(selector, imp);
	_add_methods(cls, &m, 1);
	return YES;
}
void objc_class_add_methods(Class cl, Method *m, unsigned int count){
	_add_methods(cl, m, count);
}

#pragma mark -
#pragma mark Replacing methods

IMP class_replaceMethod(Class cls, SEL name, IMP imp){
	if (cls == Nil || name == 0 || imp == NULL){
		return NULL;
	}
	
	Method m = class_getMethod(cls, name);
	if (m == NULL){
		Method new_method = objc_method_create(name, imp);
		_add_methods(cls, &new_method, 1);
	}else{
		m->implementation = imp;
		++m->version;
	}
	return m == NULL ? NULL : m->implementation;
}

#include "method.h"
#include "selector.h" /* For objc_selector_register. */
#include "os.h" /* For objc_alloc. */
#include "utils.h" /* For objc_strcpy */

/* Public functions are documented in the header file. */

Method objc_method_create(SEL selector, IMP implementation){
	objc_assert(implementation != NULL, "Trying to create a method with NULL implementation!");
	
	Method m = objc_alloc(sizeof(struct objc_method));
	m->selector = selector;
	m->implementation = implementation;
	m->version = 0;
	return m;
}

IMP objc_method_get_implementation(Method method){
	objc_assert(method != NULL, "Getting implementation of NULL method!");
	return method->implementation;
}


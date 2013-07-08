#include "method.h"
#include "selector.h" /* For objc_selector_register. */
#include "os.h" /* For objc_alloc. */
#include "utils.h" /* For objc_strcpy */

#pragma mark -
#pragma mark Private functions

/**
 * Returns a number of methods in a method list.
 */
OBJC_INLINE unsigned int _method_count_in_method_list(objc_array list) OBJC_ALWAYS_INLINE;
OBJC_INLINE unsigned int _method_count_in_method_list(objc_array list){
	unsigned int count = 0;
	objc_array_enumerator en;
	
	if (list == NULL){
		return count;
	}
	
	en = objc_array_get_enumerator(list);
	while (en != NULL) {
		Method *methods = en->item;
		while (*methods != NULL){
			++count;
			++methods;
		}
		en = en->next;
	}
	return count;
}


/**
 * Copies methods from method_list to list.
 */
OBJC_INLINE void _methods_copy_to_list(objc_array method_list, Method *list, unsigned int max_count) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _methods_copy_to_list(objc_array method_list, Method *list, unsigned int max_count){
	unsigned int count = 0;
	objc_array_enumerator en;
	
	if (method_list == NULL){
		/* NULL-terminate, even when NULL */
		list[0] = NULL;
		return;
	}
	
	en = objc_array_get_enumerator(method_list);
	while (en != NULL) {
		Method *methods = en->item;
		while (*methods != NULL){
			list[count] = *methods;
			++count;
			++methods;
		}
		en = en->next;
	}
	
	/* NULL termination */
	list[max_count] = NULL;
}

#pragma mark -
#pragma mark Public functions

/* Public functions are documented in the header file. */

Method objc_method_create(SEL selector, const char *types, IMP implementation){
	Method m = objc_alloc(sizeof(struct objc_method));
	m->selector = selector;
	m->implementation = implementation;
	m->types = objc_strcpy(types);
	m->version = 0;
	return m;
}

IMP objc_method_get_implementation(Method method){
	return method->implementation;
}

objc_array objc_method_transform_method_prototypes(struct objc_method_prototype **prototypes){
	objc_array arr;
	Method *methods;
	unsigned int i = 0;
	
	if (prototypes == NULL){
		return NULL;
	}
	
	arr = objc_array_create();
	
	while (prototypes[i] != NULL) {
		struct objc_method_prototype *prototype = prototypes[i];
		Method m = (Method)prototype;
		
		m->selector = objc_selector_register(prototype->selector_name);
		
		++i;
	}
	
	
	methods = (Method*)prototypes;
	objc_array_append(arr, methods);
	return arr;
}

Method *objc_method_list_flatten(objc_array method_list){
	unsigned int number_of_methods = _method_count_in_method_list(method_list);
	Method *methods = objc_alloc(sizeof(Method) * (number_of_methods + 1));
	_methods_copy_to_list(method_list, methods, number_of_methods);
	return methods;
}


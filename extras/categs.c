#include "categs.h"
#include "../classext.h"
#include "../utils.h"
#include "../selector.h"
#include "../method.h"

objc_class_extension categories_extension;

typedef struct {
	objc_array categories;
} categories_extension_class_part;


OBJC_INLINE BOOL _category_named_is_contained_in_list(const char *name, objc_array list){
	objc_array_enumerator en = objc_array_get_enumerator(list);
	while (en != NULL){
		if (objc_strings_equal(name, ((Category)(en->item))->category_name)){
			return YES;
		}
		en = en->next;
	}
	return NO;
}

BOOL objc_class_add_category(Category category){
	Class cl = objc_class_for_name(category->class_name);
	categories_extension_class_part *ext_part = (categories_extension_class_part*)objc_class_extensions_beginning_for_extension(cl, &categories_extension);
	
	if (cl == Nil){
		objc_log("Cannot register category %s for class %s since the class isn't registered with the run-time.", category->category_name, category->class_name);
		return NO;
	}
	
	/** Lazy initialization. */
	if (ext_part->categories == NULL){
		ext_part->categories = objc_array_create();
	}
	
	if (_category_named_is_contained_in_list(category->category_name, ext_part->categories)){
		objc_log("Class %s already contains a category named %s.", objc_class_get_name(cl), category->category_name);
		return NO;
	}
	
	objc_array_append(ext_part->categories, category);
	objc_class_flush_caches(cl);
	
	return YES;
}

Category objc_class_register_category_prototype(struct objc_category_prototype *category_prototype){
	Category category = (Category)category_prototype;
	category->instance_methods = objc_method_transform_method_prototypes(category_prototype->instance_methods);
	category->class_methods = objc_method_transform_method_prototypes(category_prototype->class_methods);
	
	if (objc_class_add_category(category)){
		return category;
	}
	
	return NULL;
}

OBJC_INLINE unsigned int _number_of_categories_in_class(Class cl){
	categories_extension_class_part *ext_part = (categories_extension_class_part*)objc_class_extensions_beginning_for_extension(cl, &categories_extension);
	objc_array_enumerator en;
	unsigned int count = 0;
	
	if (ext_part->categories == NULL){
		return 0;
	}
	
	en = objc_array_get_enumerator(ext_part->categories);
	while (en != NULL) {
		++count;
		en = en->next;
	}
	
	return count;
}

Category *objc_class_get_category_list(Class cl){
	categories_extension_class_part *ext_part = (categories_extension_class_part*)objc_class_extensions_beginning_for_extension(cl, &categories_extension);
	unsigned int number_of_categories = _number_of_categories_in_class(cl);
	unsigned int space_left = number_of_categories;
	Category *categories = objc_alloc((number_of_categories + 1) * sizeof(Category));
	objc_array_enumerator en;
	
	if (number_of_categories == 0){
		categories[0] = NULL;
		return categories;
	}
	
	en = objc_array_get_enumerator(ext_part->categories);
	while (en != NULL && space_left > 0) {
		categories[number_of_categories - space_left] = en->item;
		
		--space_left;
		en = en->next;
	}
	
	categories[number_of_categories] = NULL;
	
	return categories;
}

Method *objc_category_get_class_methods(Category category){
	return objc_method_list_flatten(category->class_methods);
}
Method *objc_category_get_instance_methods(Category category){
	return objc_method_list_flatten(category->instance_methods);
}
const char *objc_category_get_name(Category category){
	return category->category_name;
}


OBJC_INLINE Method _find_method_in_list(objc_array list, SEL selector){
	objc_array_enumerator en;
	
	if (list == NULL){
		return NULL;
	}
	
	en = objc_array_get_enumerator(list);
	while (en != NULL) {
		Method *methods = en->item;
		if (methods != NULL){
			while (*methods != NULL) {
				if (objc_selectors_equal(selector, (*methods)->selector)){
					return *methods;
				}
				++methods;
			}
		}
		
		en = en->next;
	}
	
	return NULL;
}

static Method instance_lookup_function(Class cl, SEL selector){
	categories_extension_class_part *ext_part = (categories_extension_class_part*)objc_class_extensions_beginning_for_extension(cl, &categories_extension);
	objc_array_enumerator en;
	
	if (ext_part->categories == NULL){
		return NULL;
	}
	
	en = objc_array_get_enumerator(ext_part->categories);
	while (en != NULL){
		Category c = en->item;
		Method m = _find_method_in_list(c->instance_methods, selector);
		if (m != NULL){
			return m;
		}
		
		en = en->next;
	}
	
	return NULL;
}
static Method class_lookup_function(Class cl, SEL selector){
	categories_extension_class_part *ext_part = (categories_extension_class_part*)objc_class_extensions_beginning_for_extension(cl, &categories_extension);
	objc_array_enumerator en;
	
	if (ext_part->categories == NULL){
		return NULL;
	}
	
	en = objc_array_get_enumerator(ext_part->categories);
	while (en != NULL){
		Category c = en->item;
		Method m = _find_method_in_list(c->class_methods, selector);
		if (m != NULL){
			return m;
		}
		
		en = en->next;
	}
	
	return NULL;
}


void objc_categories_register_extension(void){
	categories_extension.class_initializer = NULL;
	categories_extension.class_lookup_function = class_lookup_function;
	categories_extension.extra_class_space = sizeof(categories_extension);
	categories_extension.extra_object_space = 0;
	categories_extension.instance_lookup_function = instance_lookup_function;
	categories_extension.object_destructor = NULL;
	categories_extension.object_initializer = NULL; /* Lazy allocation */
	
	objc_class_add_extension(&categories_extension);
}

static void objc_categories_register_initializer(void){
	objc_runtime_register_initializer(objc_categories_register_extension);
}

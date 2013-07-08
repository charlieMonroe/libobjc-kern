
#ifndef _OBJC_CATEGORIES_H_
#define _OBJC_CATEGORIES_H_

#include "../os.h"

typedef struct objc_category {
	const char *class_name; /** Name of the class of which it is a category. */
	const char *category_name; /** Name of the category. */
	objc_array class_methods; /** The same structure as in Class. */
	objc_array instance_methods; /** The same structure as in Class. */
} * Category;

struct objc_category_prototype {
	const char *class_name;
	const char *category_name;
	struct objc_method_prototype **class_methods; /** Must be NULL-terminated. */
	struct objc_method_prototype **instance_methods; /** Must be NULL-terminated. */
};

extern BOOL objc_class_add_category(Category category);
extern Category objc_class_register_category_prototype(struct objc_category_prototype *category_prototype);
extern Category *objc_class_get_category_list(Class cl);

extern Method *objc_category_get_class_methods(Category category);
extern Method *objc_category_get_instance_methods(Category category);
extern const char *objc_category_get_name(Category category);


extern void objc_categories_register_extension(void);

#endif /* _OBJC_CATEGORIES_H_ */

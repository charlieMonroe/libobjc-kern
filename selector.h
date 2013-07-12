
#ifndef OBJC_SELECTOR_H_
#define OBJC_SELECTOR_H_

#include "types.h"
#include "os.h" // For OBJC_INLINE

/* 
 * The selector name is copied over, as well as the types.
 *
 * Trying to add a selector with an existing name, yet different types
 * yields in a runtime error.
 */
extern SEL objc_selector_register(const char *name, const char *types);

/* 
 * Since SEL is just a 16-bit number pointing into the selector table,
 * simple comparison is sufficient.
 */
OBJC_INLINE BOOL objc_selectors_equal(SEL selector1, SEL selector2){
	return selector1 == selector2;
}

/* 
 * Returns the selector name.
 */
extern const char *objc_selector_get_name(SEL selector);

/*
 * Returns the selector types.
 */
extern const char *objc_selector_get_types(SEL selector);


// TODO move to private
/**
 * Registers all selectors within the class or method list.
 */
extern void objc_register_selectors_from_method_list(objc_method_list *list);
extern void objc_register_selectors_from_class(Class cl);
extern void objc_register_selector_array(struct objc_selector *selectors, unsigned int count);

#define null_selector ((SEL)0)

#endif /* OBJC_SELECTOR_H_ */

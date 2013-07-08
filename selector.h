
#ifndef OBJC_SELECTOR_H_
#define OBJC_SELECTOR_H_

#include "types.h"
#include "os.h"
#include "utils.h"

/* The selector name is copied over */
extern SEL objc_selector_register(const char *name);

/* Pointer and name comparison */
OBJC_INLINE BOOL objc_selectors_equal(SEL selector1, SEL selector2) OBJC_ALWAYS_INLINE;
OBJC_INLINE BOOL objc_selectors_equal(SEL selector1, SEL selector2){
	if (selector1 == NULL && selector2 == NULL){
		return YES;
	}
	if (selector1 == NULL || selector2 == NULL){
		return NO;
	}
	
	return selector1 == selector2 || objc_strings_equal(selector1->name, selector2->name);
}

/* Returns the selector name */
extern const char *objc_selector_get_name(SEL selector);

#endif /* OBJC_SELECTOR_H_ */

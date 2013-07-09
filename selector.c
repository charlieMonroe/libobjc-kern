
#include "selector.h"
#include "utils.h" /* For strcpy */

/**
 * Since the SEL is a 16-bit integer, all we need to
 * have are 2-level arrays that get slowly filled as
 * needed for the look-up.
 *
 * The actual
 *
 *
 */


static objc_selector_holder selector_cache;

/* Public functions, documented in the header file. */

SEL objc_selector_register(const char *name){
	SEL selector = objc_selector_holder_lookup(selector_cache, name);
	if (selector == NULL){
		/* Check if the selector hasn't been added yet */
		selector = (SEL)objc_selector_holder_lookup(selector_cache, name);
		if (selector == NULL){
			/* Still nothing, insert */
			selector = objc_alloc(sizeof(struct objc_selector));
			selector->name = objc_strcpy(name);
			objc_selector_holder_insert(selector_cache, selector);
		}
	}
	return selector;
}

const char *objc_selector_get_name(SEL selector){
	if (selector == NULL){
		return "((null))";
	}
	return selector->name;
}

void objc_selector_init(void){
	selector_cache = objc_selector_holder_create();
}


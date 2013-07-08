
#ifndef OBJC_SELECTOR_HOLDER_H_
#define OBJC_SELECTOR_HOLDER_H_

#include "../os.h"
#if !OBJC_USES_INLINE_FUNCTIONS

#include "../types.h"

/**
 * Functions compatible with declares in function-types.h, section objc_selector_holder.
 */
extern objc_selector_holder selector_holder_create(void);
extern void selector_holder_insert_selector(objc_selector_holder holder, SEL selector);
extern SEL selector_holder_lookup_selector(objc_selector_holder holder, const char *name);


#endif /* OBJC_USES_INLINE_FUNCTIONS */
#endif /* OBJC_SELECTOR_HOLDER_H_ */

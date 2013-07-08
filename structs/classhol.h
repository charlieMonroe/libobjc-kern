
#ifndef OBJC_CLASS_HOLDER_H_
#define OBJC_CLASS_HOLDER_H_

#include "../os.h"
#if !OBJC_USES_INLINE_FUNCTIONS

#include "../types.h"

/**
 * Functions compatible with declares in function-types.h, section objc_class_holder.
 */
extern objc_class_holder class_holder_create(void);
extern void class_holder_insert_class(objc_class_holder holder, Class cl);
extern Class class_holder_lookup_class(objc_class_holder holder, const char *name);

#endif /* OBJC_USES_INLINE_FUNCTIONS */
#endif /* OBJC_CLASS_HOLDER_H_ */


#ifndef OBJC_CLASS_EXTRA_H
#define OBJC_CLASS_EXTRA_H

#include "types.h"

typedef struct objc_class_extra_struct {
	struct objc_class_extra_struct *next;
	void *data;
	unsigned int identifier;
} objc_class_extra;

#define OBJC_ASSOCIATED_OBJECTS_IDENTIFIER ((unsigned int)'aobj')

/**
 * Returns extra with a specifix identifier. Never returns NULL. If the class
 * doesn't have this extra, it adds it.
 */
PRIVATE void **objc_class_extra_with_identifier(Class cl, unsigned int identifier);


#endif


#ifndef OBJC_CLASS_EXTRA_H
#define OBJC_CLASS_EXTRA_H

#define OBJC_ASSOCIATED_OBJECTS_IDENTIFIER ((unsigned int)'aobj')

/*
 * Returns extra with a specifix identifier. Never returns NULL. If the class
 * doesn't have this extra, it adds it.
 */
PRIVATE void **	objc_class_extra_with_identifier(Class cl,
						 unsigned int identifier);


#endif /* !OBJC_CLASS_EXTRA_H */

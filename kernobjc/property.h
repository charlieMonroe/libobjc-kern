
#ifndef LIBKERNOBJC_PROPERTY_H
#define LIBKERNOBJC_PROPERTY_H

objc_property_attribute_t *property_copyAttributeList(objc_property_t property,
						      unsigned int *outCount);
char		*property_copyAttributeValue(objc_property_t property,
					     const char *attributeName);


const char	*property_getAttributes(objc_property_t property);
const char	*property_getName(objc_property_t property);

id objc_getProperty(id obj, SEL _cmd, ptrdiff_t offset, BOOL isAtomic);
void objc_setProperty(id obj, SEL _cmd, ptrdiff_t offset, id arg, BOOL isAtomic, BOOL isCopy);
void objc_setProperty_atomic(id obj, SEL _cmd, id arg, ptrdiff_t offset);
void objc_setProperty_atomic_copy(id obj, SEL _cmd, id arg, ptrdiff_t offset);
void objc_setProperty_nonatomic(id obj, SEL _cmd, id arg, ptrdiff_t offset);
void objc_setProperty_nonatomic_copy(id obj, SEL _cmd, id arg, ptrdiff_t offset);
void objc_getPropertyStruct(void *dest,
                            void *src,
                            ptrdiff_t size,
                            BOOL atomic,
                            BOOL strong);
void objc_setPropertyStruct(void *dest,
                            void *src,
                            ptrdiff_t size,
                            BOOL atomic,
                            BOOL strong);

// DEPRECATED
void objc_copyPropertyStruct(void *dest,
                             void *src,
                             ptrdiff_t size,
                             BOOL atomic,
                             BOOL strong);

#endif /* !LIBKERNOBJC_PROPERTY_H */

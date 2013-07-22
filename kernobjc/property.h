
#ifndef LIBKERNOBJC_PROPERTY_H
#define LIBKERNOBJC_PROPERTY_H

objc_property_attribute_t *property_copyAttributeList(objc_property_t property,
						      unsigned int *outCount);
char		*property_copyAttributeValue(objc_property_t property,
					     const char *attributeName);


const char	*property_getAttributes(objc_property_t property);
const char	*property_getName(objc_property_t property);


#endif /* !LIBKERNOBJC_PROPERTY_H */

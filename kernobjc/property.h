
#ifndef LIBKERNOBJC_PROPERTY_H
#define LIBKERNOBJC_PROPERTY_H

const char *property_getName(objc_property_t property);
const char *property_getAttributes(objc_property_t property);
objc_property_attribute_t *property_copyAttributeList(objc_property_t property, unsigned int *outCount);
char *property_copyAttributeValue(objc_property_t property, const char *attributeName);


#endif

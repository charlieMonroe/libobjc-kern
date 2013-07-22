
#ifndef LIBKERNOBJC_PROTOCOL_H
#define LIBKERNOBJC_PROTOCOL_H

BOOL		protocol_conformsToProtocol(Protocol *proto, Protocol *other);
BOOL		protocol_isEqual(Protocol *proto, Protocol *other);
const char	*protocol_getName(Protocol *p);
Protocol	*objc_allocateProtocol(const char *name);
void		objc_registerProtocol(Protocol *proto);
void		protocol_addMethodDescription(Protocol *proto, SEL name,
					      const char *types,
					      BOOL isRequiredMethod,
					      BOOL isInstanceMethod);
void		protocol_addProtocol(Protocol *proto, Protocol *addition);
void		protocol_addProperty(Protocol *proto, const char *name,
				     const objc_property_attribute_t *attributes,
				     unsigned int attributeCount,
				     BOOL isRequiredProperty,
				     BOOL isInstanceProperty);


struct objc_method_description
protocol_getMethodDescription(Protocol *p, SEL aSel,
			      BOOL isRequiredMethod, BOOL isInstanceMethod);

struct objc_method_description *
protocol_copyMethodDescriptionList(Protocol *p,
				   BOOL isRequiredMethod,
				   BOOL isInstanceMethod,
				   unsigned int *outCount);

objc_property_t	protocol_getProperty(Protocol *proto,
				     const char *name,
				     BOOL isRequiredProperty,
				     BOOL isInstanceProperty);

objc_property_t *protocol_copyPropertyList(Protocol *proto,
					   unsigned int *outCount);

Protocol * __unsafe_unretained *
protocol_copyProtocolList(Protocol *proto, unsigned int *outCount);


#endif /* !LIBKERNOBJC_PROTOCOL_H */

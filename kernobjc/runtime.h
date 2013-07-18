
#ifndef LIBKERNOBJC_RUNTIME_H
#define LIBKERNOBJC_RUNTIME_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "types.h"
#include "selector.h"
#include "object.h"
#include "class.h"




/**
 * Finds a class registered with the run-time and returns it,
 * or Nil, if no such class is registered.
 *
 * Before returning Nil, however, it checks the hook
 * if it can supply a class (unlike the objc_lookUpClass()).
 */
id objc_getClass(const char *name);
id objc_lookUpClass(const char *name);

/**
 * Returns a list of classes registered with the run-time. 
 * The caller is responsible for freeing
 * it using the free function.
 */
int objc_getClassList(Class *buffer, int bufferCount);
Class *objc_copyClassList(unsigned int *outCount);


id objc_getMetaClass(const char *name);
id objc_getRequiredClass(const char *name);


Protocol *objc_getProtocol(const char *name);
Protocol*__unsafe_unretained* objc_copyProtocolList(Protocol *protocol, unsigned int *outCount);



Class objc_allocateClassPair(Class superclass, const char *name,
			     size_t extraBytes);
void objc_registerClassPair(Class cls);
Class objc_duplicateClass(Class original, const char *name,
			  size_t extraBytes);
void objc_disposeClassPair(Class cls);

SEL method_getName(Method m);
IMP method_getImplementation(Method m);
const char *method_getTypeEncoding(Method m);

unsigned int method_getNumberOfArguments(Method m);
char *method_copyReturnType(Method m);
char *method_copyArgumentType(Method m, unsigned int index);
void method_getReturnType(Method m, char *dst, size_t dst_len);
void method_getArgumentType(Method m, unsigned int index,
			    char *dst, size_t dst_len);
struct objc_method_description *method_getDescription(Method m);

IMP method_setImplementation(Method m, IMP imp);
void method_exchangeImplementations(Method m1, Method m2);

const char *ivar_getName(Ivar v);
const char *ivar_getTypeEncoding(Ivar v);
ptrdiff_t ivar_getOffset(Ivar v);

const char *property_getName(objc_property_t property);
const char *property_getAttributes(objc_property_t property);
objc_property_attribute_t *property_copyAttributeList(objc_property_t property, unsigned int *outCount);
char *property_copyAttributeValue(objc_property_t property, const char *attributeName);

BOOL protocol_conformsToProtocol(Protocol *proto, Protocol *other);
BOOL protocol_isEqual(Protocol *proto, Protocol *other);
const char *protocol_getName(Protocol *p);
struct objc_method_description protocol_getMethodDescription(Protocol *p, SEL aSel, BOOL isRequiredMethod, BOOL isInstanceMethod);
struct objc_method_description *protocol_copyMethodDescriptionList(Protocol *p, BOOL isRequiredMethod, BOOL isInstanceMethod, unsigned int *outCount);
objc_property_t protocol_getProperty(Protocol *proto, const char *name, BOOL isRequiredProperty, BOOL isInstanceProperty);
objc_property_t *protocol_copyPropertyList(Protocol *proto, unsigned int *outCount);
Protocol * __unsafe_unretained *protocol_copyProtocolList(Protocol *proto, unsigned int *outCount);

Protocol *objc_allocateProtocol(const char *name);
void objc_registerProtocol(Protocol *proto);
void protocol_addMethodDescription(Protocol *proto, SEL name, const char *types, BOOL isRequiredMethod, BOOL isInstanceMethod);
void protocol_addProtocol(Protocol *proto, Protocol *addition);
void protocol_addProperty(Protocol *proto, const char *name, const objc_property_attribute_t *attributes, unsigned int attributeCount, BOOL isRequiredProperty, BOOL isInstanceProperty);

const char **objc_copyImageNames(unsigned int *outCount);
const char *class_getImageName(Class cls);
const char **objc_copyClassNamesForImage(const char *image,
					 unsigned int *outCount);

void objc_enumerationMutation(id);
void objc_setEnumerationMutationHandler(void (*handler)(id));

void objc_setForwardHandler(void *fwd, void *fwd_stret);

IMP imp_implementationWithBlock(id block);
id imp_getBlock(IMP anImp);
BOOL imp_removeBlock(IMP anImp);


/* Associated Object support. */


void objc_setAssociatedObject(id object, const void *key, id value, objc_AssociationPolicy policy);
id objc_getAssociatedObject(id object, const void *key);
void objc_removeAssociatedObjects(id object);


// API to be called by clients of objects

id objc_loadWeak(id *location);
// returns value stored (either obj or NULL)
id objc_storeWeak(id *location, id obj);


#endif // LIBKERNOBJC_RUNTIME_H

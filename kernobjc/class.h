
#ifndef LIBKERNOBJC_CLASS_H
#define LIBKERNOBJC_CLASS_H

/**
 * Returns the name of the class.
 */
const char *class_getName(Class cls);

/**
 * Returns true if it is a meta class.
 */
BOOL class_isMetaClass(Class cls);

/**
 * Returns the classes superclass, 
 */
Class class_getSuperclass(Class cls);

/**
 * Returns size of an instance of a class.
 */
size_t class_getInstanceSize(Class cls);

/**
 * Returns an ivar for name.
 */
Ivar class_getInstanceVariable(Class cls, const char *name);

/**
 * Returns a list of ivars. The caller is responsible for
 * freeing it using the free function.
 *
 * Note that this function only returns ivars that are
 * declared directly on the class. Ivars declared
 * on the class' superclasses are not included.
 */
Ivar *class_copyIvarList(Class cls, unsigned int *outCount);


Method class_getMethod(Class cls, SEL name);
Method class_getInstanceMethod(Class cls, SEL name);
Method class_getInstanceMethodNonRecursive(Class cls, SEL name);
Method class_getClassMethod(Class cls, SEL name);
IMP class_getMethodImplementation(Class cls, SEL name);
IMP class_getMethodImplementation_stret(Class cls, SEL name);

/**
 * The following function returns a list of methods
 * implemented on a class. The caller is responsible 
 * for freeing it using the free function.
 *
 * Note that this function only returns methods that are
 * implemented directly on the class. Methods implemented
 * on the class' superclasses are not included.
 */
Method *class_copyMethodList(Class cls, unsigned int *outCount);

BOOL class_respondsToSelector(Class cls, SEL sel);

BOOL class_conformsToProtocol(Class cls, Protocol *protocol);
Protocol * __unsafe_unretained *class_copyProtocolList(Class cls, unsigned int *outCount);

objc_property_t class_getProperty(Class cls, const char *name);
objc_property_t *class_copyPropertyList(Class cls, unsigned int *outCount);


// TODO
const uint8_t *class_getIvarLayout(Class cls);
void class_setIvarLayout(Class cls, const uint8_t *layout);

// TODO
const uint8_t *class_getWeakIvarLayout(Class cls);
void class_setWeakIvarLayout(Class cls, const uint8_t *layout);


/**
 * The following functions add methods or a single method to the class.
 *
 * If a method with the same selector is already attached to the class,
 * this doesn't override it. This is due to the method lists being a linked
 * list and the new methods being attached to the end of the list
 * as well as maintaining behavior of other run-times.
 */
BOOL class_addMethod(Class cls, SEL name, IMP imp);

/**
 * Replaces a method implementation for another one.
 *
 * Returns the old implementation.
 */
IMP class_replaceMethod(Class cls, SEL name, IMP imp);

/**
 * Adds an ivar to a class. If the class is resolved,
 * calling this function aborts the program.
 */
BOOL class_addIvar(Class cls, const char *name, size_t size,
		   uint8_t alignment, const char *types);



BOOL class_addProtocol(Class cls, Protocol *protocol);


BOOL class_addProperty(Class cls, const char *name, const objc_property_attribute_t *attributes, unsigned int attributeCount);

void class_replaceProperty(Class cls, const char *name, const objc_property_attribute_t *attributes, unsigned int attributeCount);


#endif

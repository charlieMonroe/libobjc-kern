
#ifndef OBJC_METHOD_H_
#define OBJC_METHOD_H_

#include "types.h"

/**
 * Allocates a new Method structure and populates it with the arguments.
 *
 * Unlike other implementations, we don't need types since they already
 * are a part of the selector.
 */
extern Method objc_method_create(SEL selector, IMP implementation);

/**
 * Returns the IMP of the method.
 */
extern IMP objc_method_get_implementation(Method method);


#pragma mark -
#pragma mark Adding methods

/**
 * The following functions add methods or a single method to the class.
 *
 * If a method with the same selector is already attached to the class,
 * this doesn't override it. This is due to the method lists being a linked
 * list and the new methods being attached to the end of the list
 * as well as maintaining behavior of other run-times.
 *
 * Caller is responsible for the method 'objects' to be kept in memory,
 * however, the method array does get copied. In particular, when you
 * call the add_*_methods function, you are responsible for each *m,
 * however, you may free(m) when done.
 */
extern void objc_class_add_method(Class cl, Method m);
extern void objc_class_add_methods(Class cl, Method *m, unsigned int count);


#pragma mark -
#pragma mark Replacing methods

/**
 * Replaces a method implementation for another one.
 *
 * Returns the old implementation.
 */
extern IMP objc_class_replace_method_implementation(Class cls, SEL name, IMP imp, const char *types);

#endif /* OBJC_METHOD_H_ */

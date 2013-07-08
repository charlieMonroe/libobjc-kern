
#ifndef OBJC_METHOD_H_
#define OBJC_METHOD_H_

#include "types.h"

/**
 * Allocates a new Method structure and populates it with the arguments.
 */
extern Method objc_method_create(SEL selector, const char *types, IMP implementation);

/**
 * Returns the IMP of the method.
 */
extern IMP objc_method_get_implementation(Method method);

/**
 * Returns an objc_array of Method pointers instead of the prototypes.
 *
 * Suitable e.g. for Class->instance_methods or Class->class_methods.
 */
extern objc_array objc_method_transform_method_prototypes(struct objc_method_prototype **prototypes);

/**
 * Creates a Method array out of an array of arrays.
 */
extern Method *objc_method_list_flatten(objc_array method_list);

#endif /* OBJC_METHOD_H_ */

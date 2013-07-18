
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

#endif /* OBJC_METHOD_H_ */

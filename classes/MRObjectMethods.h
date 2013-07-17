
#ifndef _MRObjectMethods_H_
#define _MRObjectMethods_H_

#include "../types.h"
#include "MRObjects.h"

id _C_MRObject_alloc_(id self, SEL _cmd);
void _C_MRObject_load_(id self, SEL _cmd);
id _C_MRObject_new_(id self, SEL _cmd);
void _C_MRObject_initialize_(id self, SEL _cmd);
id _C_MRObject_retain_noop_(Class self, SEL _cmd);
void _C_MRObject_release_noop_(Class self, SEL _cmd);

id _I_MRObject_init_(MRObject_instance_t *self, SEL _cmd);

id _I_MRObject_retain_(MRObject_instance_t *self, SEL _cmd);
void _I_MRObject_release_(MRObject_instance_t *self, SEL _cmd);
void _I_MRObject_dealloc_(MRObject_instance_t *self, SEL _cmd);

const char *_I___MRConstString_cString_(__MRConstString_instance_t *self, SEL _cmd);
unsigned int _I___MRConstString_length_(__MRConstString_instance_t *self, SEL _cmd);

#endif /** _MRObjectMethods_H_ */

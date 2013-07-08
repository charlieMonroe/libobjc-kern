
#ifndef _MRObjectMethods_H_
#define _MRObjectMethods_H_

#include "../types.h"
#include "MRObjects.h"

extern id _C_MRObject_alloc_(id self, SEL _cmd);
extern id _C_MRObject_new_(id self, SEL _cmd);
extern id _C_MRObject_retain_noop_(Class self, SEL _cmd);
extern void _C_MRObject_release_noop_(Class self, SEL _cmd);

extern id _I_MRObject_init_(MRObject_instance_t *self, SEL _cmd);

extern id _I_MRObject_retain_(MRObject_instance_t *self, SEL _cmd);
extern void _I_MRObject_release_(MRObject_instance_t *self, SEL _cmd);
extern void _I_MRObject_dealloc_(MRObject_instance_t *self, SEL _cmd);

extern Method _IC_MRObject_forwardedMethodForSelector_(MRObject_instance_t *self, SEL _cmd, SEL selector);
extern BOOL _IC_MRObject_dropsUnrecognizedMessage_(MRObject_instance_t *self, SEL _cmd, SEL selector);

extern const char *_I___MRConstString_cString_(__MRConstString_instance_t *self, SEL _cmd);
extern unsigned int _I___MRConstString_length_(__MRConstString_instance_t *self, SEL _cmd);

#endif /** _MRObjectMethods_H_ */

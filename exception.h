
#ifndef OBJC_EXCEPTION_H_
#define OBJC_EXCEPTION_H_

#include "unwind/unwind.h"

void objc_exception_throw(id object);
void objc_exception_rethrow(struct _Unwind_Exception *e);

id objc_begin_catch(struct _Unwind_Exception *exceptionObject);
void objc_end_catch(void);

_Unwind_Reason_Code __kern_objc_personality_v0(int version,
											   _Unwind_Action actions,
											   uint64_t exceptionClass,
											   struct _Unwind_Exception *exceptionObject,
											   struct _Unwind_Context *context);

#endif /* !OBJC_EXCEPTION_H_ */

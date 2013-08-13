
#ifndef OBJC_EXCEPTION_H_
#define OBJC_EXCEPTION_H_

#include "unwind/unwind.h"

void objc_exception_rethrow(struct _Unwind_Exception *e);

id objc_begin_catch(struct _Unwind_Exception *exceptionObject);
void objc_end_catch(void);


#endif /* !OBJC_EXCEPTION_H_ */

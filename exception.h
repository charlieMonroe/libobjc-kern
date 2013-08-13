
#ifndef OBJC_EXCEPTION_H_
#define OBJC_EXCEPTION_H_

#include "unwind/unwind.h"

void objc_exception_rethrow(struct _Unwind_Exception *e);

id objc_begin_catch(struct _Unwind_Exception *exceptionObject);
void objc_end_catch(void);


#define EXCEPTION_CLASS(a,b,c,d,e,f,g,h)																						\
			((((uint64_t)a) << 56) + (((uint64_t)b) << 48) + (((uint64_t)c) << 40) +							\
			(((uint64_t)d) << 32) + (((uint64_t)e) << 24) + (((uint64_t)f) << 16) +								\
			(((uint64_t)g) << 8) + (((uint64_t)h)))

#endif /* !OBJC_EXCEPTION_H_ */

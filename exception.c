#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "exception.h"
#include "init.h"
#include "kernobjc/runtime.h"
#include "private.h"
#include "class.h"

objc_tls_key objc_exception_tls_key;


/*
 * Clang enjoys to for whatever reason to add two required symbols to each 
 * binary that throws / catches exceptions, even though it is strictly said
 * that the runtime doesn't use libunwind exceptions. Even though both symbols
 * are declared as weak, the kernel doesn't really do weak...
 *
 * I was so far unable to identify which part of clang emits these symbols, but
 * they both abort in either case, so if they ever get called, we're soon to 
 * find out.
 *
 * - __libkern_personality_v0 - it needs to implement *some* personality. In 
 *								case I ever get libunwind into the kernel, it's
 *								a good idea to declare our own personality.
 *
 * - _Unwind_Resume - even though I've searched for this symbol through the 
 *						whole LLVM project and the only possibility is that it
 *						gets emitted in DwarfEHPrepare.cpp
 */

void	__libkern_personality_v0(void);
void	_Unwind_Resume(void);


void
__libkern_personality_v0(void){
	objc_abort("Called __libkern_personality_v0\n");
}
void
_Unwind_Resume(void){
	objc_abort("Called _Unwind_Resume\n");
}

struct objc_exception_handler *
objc_installed_exception_handler(void)
{
	void *tls_data = objc_get_tls_for_key(objc_exception_tls_key);
	return (struct objc_exception_handler*)tls_data;
}

void
objc_exception_try_enter(struct objc_exception_handler *handler)
{
	objc_debug_log("%s - %p\n", __FUNCTION__, handler);
	
	struct objc_exception_handler *h;
	h = objc_installed_exception_handler();
	handler->previous = h;
	objc_set_tls_for_key(handler, objc_exception_tls_key);
}

void objc_exception_throw(id exception){
	objc_debug_log("Throwing exception %p [%s]\n",
				   exception, exception == nil ? "null" :
				   object_getClassName(exception));
	
	struct objc_exception_handler *handler;
	handler = objc_installed_exception_handler();
	if (handler == NULL){
		objc_abort("Unhandled exception %s!\n",
				   object_getClassName(exception));
		/* Not reached */
	}
	
	/* Set the exception. */
	handler->exception = exception;
	
	/* Now pop the top handler. */
	objc_set_tls_for_key(handler->previous, objc_exception_tls_key);
	
	/* Jump. */
	longjmp(handler->jump_buffer, 1);
}

void
objc_exception_try_exit(struct objc_exception_handler *handler)
{
	objc_debug_log("%s - %p\n", __FUNCTION__, handler);
	
	struct objc_exception_handler *h;
	h = objc_installed_exception_handler();
	objc_assert(h == handler, "Removing a handler that's not on top of the "
				"list!\n");
	
	objc_set_tls_for_key(handler->previous, objc_exception_tls_key);
}

id
objc_exception_extract(struct objc_exception_handler *handler)
{
	objc_debug_log("%s - %p\n", __FUNCTION__, handler);
	
	objc_assert(handler != NULL, "Cannot extract an exception from a "
				"NULL handler!\n");
	return handler->exception;
}

int
objc_exception_match(Class exceptionClass, id exception)
{
	objc_debug_log("Comparing exceptionClass %s vs class of object: %s\n",
				   class_getName(exceptionClass),
				   object_getClassName(exception));
	Class cl = objc_object_get_class_inline(exception);
	while (cl != Nil){
		if (cl == exceptionClass){
			objc_debug_log("Found the class for the exception %s\n",
						   class_getName(cl));
			return 1;
		}
		cl = class_getSuperclass(cl);
	}
	objc_debug_log("Couldn't find a class for the exception %p\n", exception);
	return 0;
}

void
objc_enumerationMutation(id obj)
{
	objc_abort("Mutation occurred while enumeration object %p of class %s!\n",
			   obj, object_getClassName(obj));
}


PRIVATE void
objc_exceptions_init(void)
{
	objc_register_tls(&objc_exception_tls_key, NULL);
}


PRIVATE void
objc_exceptions_destroy(void)
{
	objc_deregister_tls(objc_exception_tls_key);
}



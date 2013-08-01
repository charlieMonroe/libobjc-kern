

#include "exception.h"
#include "init.h"
#include "kernobjc/runtime.h"

objc_tls_key objc_exception_tls_key;

struct objc_exception_handler *
objc_installed_exception_handler(void)
{
	void *tls_data = objc_get_tls_for_key(objc_exception_tls_key);
	return (struct objc_exception_handler*)tls_data;
}

void
objc_install_exception_handler(struct objc_exception_handler *handler)
{
	objc_set_tls_for_key(handler, objc_exception_tls_key);
}

void objc_throw_exception(id exception){
	if (exception == nil){
		return;
	}
	
	struct objc_exception_handler *handler;
	handler = objc_installed_exception_handler();
	if (handler == NULL){
		objc_abort("Unhandled exception %s!\n",
			   object_getClassName(exception));
		/* Not reached */
	}
	
	/* Set the exception. */
	handler->exception = exception;
	
	/* Jump. */
	longjmp(handler->jump_buffer, 1);
}


PRIVATE void
objc_exceptions_init(void)
{
	objc_register_tls(&objc_exception_tls_key, NULL);
}


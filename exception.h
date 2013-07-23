
#ifndef OBJC_EXCEPTION_H_
#define OBJC_EXCEPTION_H_

#include "types.h"

struct objc_exception_handler {
	/* Points to the previous structure on stack. */
	struct objc_exception_handler *previous;
	
	/* The exception that was thrown. */
	id exception;
	
	/* jmp_buf for setjmp, longjmp. */
	jmp_buf jump_buffer;
};

/* 
 * Returns a pointer to the last objc_exception_handler structure on the stack,
 * or NULL, if there is none.
 */
struct objc_exception_handler *objc_installed_exception_handler(void);

/*
 * Sets the last objc_exception_handler structure pointer to point to install.
 */
void objc_install_exception_handler(struct objc_exception_handler *handler);

/*
 * This throws exception unless exception == nil. It looks into TLS, if any
 * exception handler is installed and if not, aborts the whole program.
 *
 * If an exception handler is installed, it the exception is asigned and
 * longjmp is invoked. 
 */
void objc_throw_exception(id exception);


#define TRY_CATCH_FINALLY(TRY, CATCH, FINALLY)				\
{									\
	struct objc_exception_handler __exc_handler;			\
	__exc_handler.previous = objc_installed_exception_handler();	\
	__exc_handler.exception = nil;					\
	objc_install_exception_handler(&__exc_handler);			\
	int result = setjmp(__exc_handler.jump_buffer);			\
	if (result == 0){						\
		/* Result 0 means the first pass -> try. */		\
		{ TRY }							\
	}else{								\
		/* Getting into the catch block. */			\
		id exception = __exc_handler.exception;			\
		CATCH							\
		else{							\
			/* Not caught by any of the catch statements. */\
			{						\
				/* Restore the previous handler. */	\
				objc_install_exception_handler(		\
					__exc_handler.previous);	\
				FINALLY					\
			}						\
			objc_throw_exception(__exc_handler.exception);	\
		}							\
	}								\
	{								\
		/* Need to restore the previous handler. */		\
		objc_install_exception_handler(__exc_handler.previous);	\
		FINALLY							\
	}								\
}

#endif /* !OBJC_EXCEPTION_H_ */


#ifndef OBJC_EXCEPTION_H_
#define OBJC_EXCEPTION_H_

struct objc_exception_handler {
	/* jmp_buf for setjmp, longjmp. */
	jmp_buf jump_buffer;
	
	/* Points to the previous structure on stack. */
	struct objc_exception_handler *previous;
	
	/* The exception that was thrown. */
	id    exception;
	
	/* Reserving two pointers for future use. */
	void  *reserved1;
	void  *reserved2;
};


/*
 * Returns a pointer to the last objc_exception_handler structure on the stack,
 * or NULL, if there is none.
 */
struct objc_exception_handler *objc_installed_exception_handler(void);

/* Sets the last objc_exception_handler structure pointer to point to install.*/
void objc_exception_try_enter(struct objc_exception_handler *handler);

/* Removes the handler from the EH chain. Must be the top handler installed! */
void objc_exception_try_exit(struct objc_exception_handler *handler);

/* Returns handler->exception. */
id objc_exception_extract(struct objc_exception_handler *handler);

/* Returns 1 if the exception is of class exceptionClass. */
int objc_exception_match(Class exceptionClass, id exception);

/*
 * This throws an exception even if nil. It looks into TLS, if any exception
 * handler is installed and if not, aborts the whole program.
 *
 * If an exception handler is installed, it the exception is asigned and
 * longjmp is invoked.
 */
void objc_exception_throw(id exception);


#endif /* !OBJC_EXCEPTION_H_ */

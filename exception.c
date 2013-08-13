#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "exception.h"
#include "init.h"
#include "kernobjc/runtime.h"
#include "class.h"
#include "private.h"
#include "unwind/dwarf_eh.h"

enum exception_type
{
	NONE,
	CXX, /* NOT SUPPORTED */
	OBJC,
	FOREIGN,
	BOXED_FOREIGN
};

/* TLS data structure */
struct thread_data
{
	enum exception_type current_exception_type;
	struct objc_exception *caughtExceptions;
};

/**
 * Structure used as a header on thrown exceptions.
 */
struct objc_exception
{
	/** The selector value to be returned when installing the catch handler.
	 * Used at the call site to determine which catch() block should execute.
	 * This is found in phase 1 of unwinding then installed in phase 2.*/
	int handlerSwitchValue;
	
	/** The cached landing pad for the catch handler.*/
	void *landingPad;
	
	/**
	 * Next pointer for chained exceptions.
	 */
	struct objc_exception *next;
	
	/**
	 * The number of nested catches that may hold this exception.  This is
	 * negative while an exception is being rethrown.
	 */
	int catch_count;
	
	/** The language-agnostic part of the exception header. */
	struct _Unwind_Exception unwindHeader;
	
	/** Thrown object.  This is after the unwind header so that the C++
	 * exception handler can catch this as a foreign exception. */
	id object;
	
	/** C++ exception structure.  Used for mixed exceptions.  When we are in
	 * Objective-C++ code, we create this structure for passing to the C++
	 * exception personality function.  It will then handle installing
	 * exceptions for us.  
	 *
	 * NOT SUPPORTED in kernel runtime.
	 */
	struct _Unwind_Exception *cxx_exception;
};

/**
 * Class of exceptions to distinguish between this and other exception types.
 */
static const uint64_t objc_exception_class = EXCEPTION_CLASS('K','R','N',
																						'C','O','B','J','C');

/* Hooks for handling exceptions. */
void(*_objc_unexpected_exception)(id exc);
Class(*_objc_class_for_boxing_foreign_exception)(uint64_t cl);

#pragma mark -
#pragma mark TLS Related

/* TLS key. */
objc_tls_key objc_exception_tls_key;

/* Passed as the TLS destructor in exception init. */
static void
_objc_exceptions_remove_tls(void *data){
	if (data != NULL){
		objc_dealloc(data, M_EXCEPTION_TYPE);
	}
}

/* Gets the TLS and creates it if required. */
static inline struct thread_data *
_objc_exception_get_thread_data(void)
{
	struct thread_data *td = objc_get_tls_for_key(objc_exception_tls_key);
	if (td == NULL) {
		td = objc_zero_alloc(sizeof(struct thread_data), M_EXCEPTION_TYPE);
		objc_set_tls_for_key(td, objc_exception_tls_key);
	}
	return td;
}

/*
 * The same as above, but may return NULL if the data hasn't been installed yet.
 */
static inline struct thread_data *
_objc_exception_get_thread_data_fast(void)
{
	return objc_get_tls_for_key(objc_exception_tls_key);
}

#pragma mark -
#pragma mark Private functions

static struct objc_exception *
objc_exception_from_header(struct _Unwind_Exception *ex)
{
	return (struct objc_exception*)((char*)ex -
									offsetof(struct objc_exception, unwindHeader));
}

static BOOL
isKindOfClass(Class thrown, Class type)
{
	do
	{
		if (thrown == type)
		{
			return YES;
		}
		thrown = class_getSuperclass(thrown);
	} while (Nil != thrown);
	
	return NO;
}


#pragma mark -
#pragma mark Personality functions

static inline _Unwind_Reason_Code
internal_objc_personality(int version,
												_Unwind_Action actions,
												uint64_t exceptionClass,
												struct _Unwind_Exception *exceptionObject,
												struct _Unwind_Context *context)
{
	objc_debug_log("New personality function called %p\n", exceptionObject);
	
	struct objc_exception *ex = NULL;
	
	char *cls = (char*)&exceptionClass;
	objc_debug_log("Class: %c%c%c%c%c%c%c%c\n", cls[7], cls[6], cls[5], cls[4],
								cls[3], cls[2], cls[1], cls[0]);
	
	// Check if this is a foreign exception.  If it is a C++ exception, then we
	// have to box it.  If it's something else, like a LanguageKit exception
	// then we ignore it (for now)
	BOOL foreignException = exceptionClass != objc_exception_class;
	// Is this a C++ exception containing an Objective-C++ object?
	BOOL objcxxException = NO;
	// The object to return
	void *object = NULL;
		
	Class thrown_class = Nil;
	
	// If it's not a foreign exception, then we know the layout of the
	// language-specific exception stuff.
	if (!foreignException){
		ex = objc_exception_from_header(exceptionObject);
		thrown_class = objc_object_get_class_inline(ex->object);
	}else if (_objc_class_for_boxing_foreign_exception){
		thrown_class = _objc_class_for_boxing_foreign_exception(exceptionClass);
		objc_debug_log("Foreign class: %p\n", thrown_class);
	}
	
	unsigned char *lsda_addr = (void*)_Unwind_GetLanguageSpecificData(context);
	objc_debug_log("LSDA: %p\n", lsda_addr);
	
	// No LSDA implies no landing pads - try the next frame
	if (NULL == lsda_addr) { return _URC_CONTINUE_UNWIND; }
	
	// These two variables define how the exception will be handled.
	struct dwarf_eh_action action = {0};
	unsigned long selector = 0;
	if (actions & _UA_SEARCH_PHASE){
		objc_debug_log("Search phase...\n");
		struct dwarf_eh_lsda lsda = parse_lsda(context, lsda_addr);
		action = dwarf_eh_find_callsite(context, &lsda);
		handler_type handler = check_action_record(context, foreignException,
												   &lsda, action.action_record, thrown_class, &selector);
		objc_debug_log("handler: %d\n", handler);
		// If there's no action record, we've only found a cleanup, so keep
		// searching for something real
		if (handler == handler_class ||
			((handler == handler_catchall_id) && !foreignException) ||
			(handler == handler_catchall))
		{
			saveLandingPad(context, exceptionObject, ex, selector, action.landing_pad);
			objc_debug_log("Found handler! %d\n", handler);
			return _URC_HANDLER_FOUND;
		}
		return _URC_CONTINUE_UNWIND;
	}
	
	objc_debug_log("Phase 2: Fight!\n");
	
	if (!(actions & _UA_HANDLER_FRAME)){
		objc_debug_log("Not the handler frame, looking up the cleanup again\n");
		struct dwarf_eh_lsda lsda = parse_lsda(context, lsda_addr);
		action = dwarf_eh_find_callsite(context, &lsda);
		
		// If there's no cleanup here, continue unwinding.
		if (0 == action.landing_pad) {
			return _URC_CONTINUE_UNWIND;
		}
		
		handler_type handler = check_action_record(context, foreignException,
												   &lsda, action.action_record, thrown_class, &selector);
		objc_debug_log("handler! %d %d\n", (int)handler,  (int)selector);
		
		// If this is not a cleanup, ignore it and keep unwinding.
		//if (check_action_record(context, foreignException, &lsda,
		//action.action_record, thrown_class, &selector) != handler_cleanup)
		if (handler != handler_cleanup){
			DEBUG_LOG("Ignoring handler! %d\n",handler);
			return _URC_CONTINUE_UNWIND;
		}
		
		objc_debug_log("Installing cleanup...\n");
		
		// If there is a cleanup, we need to return the exception structure
		// (not the object) to the calling frame.  The exception object
		object = exceptionObject;
	}else if (foreignException){
		struct dwarf_eh_lsda lsda = parse_lsda(context, lsda_addr);
		action = dwarf_eh_find_callsite(context, &lsda);
		check_action_record(context, foreignException, &lsda,
							action.action_record, thrown_class, &selector);
		
		// If it's a foreign exception, then box it.  If it's an Objective-C++
		// exception, then we need to delete the exception object.
		if (foreignException){
			objc_debug_log("Doing the foreign exception thing...\n");
			//[thrown_class exceptionWithForeignException: exceptionObject];
			SEL box_sel = sel_registerName("exceptionWithForeignException:");
			IMP boxfunction = objc_msg_lookup((id)thrown_class, box_sel);
		}
	}else{
		// Restore the saved info if we saved some last time.
		loadLandingPad(context, exceptionObject, ex, &selector, &action.landing_pad);
		object = ex->object;
		if (!isNew){
			objc_dealloc(ex, M_EXCEPTION_TYPE);
		}
	}
	
	_Unwind_SetIP(context, (unsigned long)action.landing_pad);
	_Unwind_SetGR(context, __builtin_eh_return_data_regno(0),
				  (unsigned long)(isNew ? exceptionObject : object));
	_Unwind_SetGR(context, __builtin_eh_return_data_regno(1), selector);
	
	objc_debug_log("Installing context, selector %d\n", (int)selector);
	return _URC_INSTALL_CONTEXT;
}


BEGIN_PERSONALITY_FUNCTION(__kern_objc_personality_v0)
	return internal_objc_personality(version, actions, exceptionClass,
								 exceptionObject, context);
}

#pragma mark -
#pragma mark Public functions

/**
 * Throws an Objective-C exception.  This function is, unfortunately, used for
 * rethrowing caught exceptions too, even in @finally() blocks.  Unfortunately,
 * this means that we have some problems if the exception is boxed.
 */
void
objc_exception_throw(id object)
{
	SEL rethrow_sel = sel_registerName("rethrow");
	Class object_class = objc_object_get_class_inline(object);
	if ((nil != object) &&
	    (class_respondsToSelector(object_class, rethrow_sel)))
	{
		objc_debug_log("Rethrowing\n");
		struct objc_slot *rethrow = objc_get_slot(object_class, rethrow_sel);
		rethrow->implementation(object, rethrow_sel);
		// Should not be reached!  If it is, then the rethrow method actually
		// didn't, so we throw it normally.
	}
	
	objc_debug_log("Throwing %p\n", object);
	
	struct objc_exception *ex = objc_zero_alloc(sizeof(struct objc_exception),
																						M_EXCEPTION_TYPE);
	
	ex->unwindHeader.exception_class = objc_exception_class;
	ex->unwindHeader.exception_cleanup = cleanup;
	
	ex->object = object;
	
	_Unwind_Reason_Code err = _Unwind_RaiseException(&ex->unwindHeader);
	objc_dealloc(ex, M_EXCEPTION_TYPE);
	if (_URC_END_OF_STACK == err && 0 != _objc_unexpected_exception)
	{
		_objc_unexpected_exception(object);
	}
	objc_debug_log("Throw returned %d\n",(int) err);
	objc_abort("");
}

id objc_begin_catch(struct _Unwind_Exception *exceptionObject)
{
	struct thread_data *td = _objc_exception_get_thread_data();
	objc_debug_log("Beginning catch %p\n", exceptionObject);
	if (exceptionObject->exception_class == objc_exception_class){
		td->current_exception_type = OBJC;
		struct objc_exception *ex = objc_exception_from_header(exceptionObject);
		if (ex->catch_count == 0){
			// If this is the first catch, add it to the list.
			ex->catch_count = 1;
			ex->next = td->caughtExceptions;
			td->caughtExceptions = ex;
		}else if (ex->catch_count < 0){
			// If this is being thrown, mark it as caught again and increment
			// the refcount
			ex->catch_count = -ex->catch_count + 1;
		}else{
			// Otherwise, just increment the catch count
			ex->catch_count++;
		}
		objc_debug_log("objc catch\n");
		return ex->object;
	}
	
	// If we have a foreign exception while we have stacked exceptions, we have
	// a problem.  We can't chain them, so we follow the example of C++ and
	// just abort.
	if (td->caughtExceptions != 0){
		// FIXME: Actually, we can handle a C++ exception if only ObjC
		// exceptions are in-flight
		objc_abort("td->caughtException != 0");
	}
	
	objc_debug_log("foreign exception catch\n");
	
	// Box if we have a boxing function.
	if (_objc_class_for_boxing_foreign_exception != NULL)
	{
		Class thrown_class =
		_objc_class_for_boxing_foreign_exception(exceptionObject->exception_class);
		
		BOOL cpu32_bit = sizeof(void*) == 4;
		const char *box_sel_types = cpu32_bit == 4 ? "@12@0:4@8" : "@24@0:8@16";
		SEL box_sel = sel_registerName("exceptionWithForeignException:",
																	box_sel_types);
		struct objc_slot *boxfunction = objc_get_slot(thrown_class, box_sel);
		if (boxfunction != NULL){
			id boxed = boxfunction->implementation((id)thrown_class, box_sel,
												   exceptionObject);
			td->caughtExceptions = (struct objc_exception*)boxed;
			td->current_exception_type = BOXED_FOREIGN;
			return boxed;
		}
	}
	
	td->current_exception_type = FOREIGN;
	td->caughtExceptions = (struct objc_exception*)exceptionObject;
	// If this is some other kind of exception, then assume that the value is
	// at the end of the exception header.
	return (id)((char*)exceptionObject + sizeof(struct _Unwind_Exception));
}

void
objc_end_catch(void)
{
	struct thread_data *td = _objc_exception_get_thread_data_fast();
	
	// If this is a boxed foreign exception then the boxing class is
	// responsible for cleaning it up
	if (td->current_exception_type == BOXED_FOREIGN){
		td->caughtExceptions = 0;
		td->current_exception_type = NONE;
		return;
	}
	
	objc_debug_log("Ending catch\n");
	
	if (td->current_exception_type == FOREIGN) {
		struct _Unwind_Exception *e = ((struct _Unwind_Exception*)td->caughtExceptions);
		e->exception_cleanup(_URC_FOREIGN_EXCEPTION_CAUGHT, e);
		td->current_exception_type = NONE;
		td->caughtExceptions = 0;
		return;
	}
	
	// Otherwise we should do the cleanup thing.  Nested catches are possible,
	// so we only clean up the exception if this is the last reference.
	objc_assert(td->caughtExceptions != 0, "td->caughtExceptions == 0\n");
	
	struct objc_exception *ex = td->caughtExceptions;
	// If this is being rethrown decrement its (negated) catch count, but don't
	// delete it even if its catch count would be 0.
	if (ex->catch_count < 0) {
		ex->catch_count++;
		return;
	}
	
	ex->catch_count--;
	if (ex->catch_count == 0){
		td->caughtExceptions = ex->next;
		objc_dealloc(ex, M_EXCEPTION_TYPE);
	}
}

void
objc_exception_rethrow(struct _Unwind_Exception *e)
{
	struct thread_data *td = _objc_exception_get_thread_data_fast();
	
	// If this is an Objective-C exception, then
	if (td->current_exception_type == OBJC){
		struct objc_exception *ex = objc_exception_from_header(e);
		objc_assert(e->exception_class == objc_exception_class);
		objc_assert(ex == td->caughtExceptions);
		objc_assert(ex->catch_count > 0);
		
		// Negate the catch count, so that we can detect that this is a
		// rethrown exception in objc_end_catch
		ex->catch_count = -ex->catch_count;
		_Unwind_Reason_Code err = _Unwind_Resume_or_Rethrow(e);
		objc_dealloc(ex, M_EXCEPTION_TYPE);
		if (_URC_END_OF_STACK == err && 0 != _objc_unexpected_exception){
			_objc_unexpected_exception(ex->object);
		}
		objc_abort("Unhandled exception %p!\n", ex);
	}
	
	if (td->current_exception_type == BOXED_FOREIGN){
		SEL rethrow_sel = sel_registerName("rethrow",
										   sizeof(void*) == 4 ? "v8@0:4" : "v16@0:8");
		id object = (id)td->caughtExceptions;
		Class object_class = objc_object_get_class_inline(object);
		if ((nil != object) &&
		    (class_respondsToSelector(object_class, rethrow_sel))){
			objc_debug_log("Rethrowing boxed exception\n");
			struct objc_slot *rethrow = objc_get_slot(object_class, rethrow_sel);
			if (rethrow != NULL){
				rethrow->implementation(object, rethrow_sel);
			}else{
				objc_abort("Object %p [%s] doesn't respond to -rethrow.\n", object,
						   class_getName(object_class));
			}
		}
	}
	
	objc_assert(e == (struct _Unwind_Exception*)td->caughtExceptions);
	_Unwind_Resume_or_Rethrow(e);
	objc_abort("");
}


PRIVATE void
objc_exceptions_init(void)
{
	objc_register_tls(&objc_exception_tls_key, _objc_exceptions_remove_tls);
}

PRIVATE void
objc_exceptions_destroy(void)
{
	objc_deregister_tls(objc_exception_tls_key);
}


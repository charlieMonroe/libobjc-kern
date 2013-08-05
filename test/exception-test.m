#include "../kernobjc/runtime.h"
#include "../exception.h"
#include "../os.h"
#include "../private.h"


static Class ExceptionClass;
static Class OtherExceptionClass;

static void run_exception_test_for_class(Class cl){
	BOOL wasInTry = NO;
	BOOL wasInFinally = NO;
	Class exceptionClass = Nil;
	
	TRY_CATCH_FINALLY({
		objc_debug_log("\t\tTry block [%s]\n", class_getName(cl));
		wasInTry = YES;
		id exception = class_createInstance(cl, 0);
		objc_throw_exception(exception);
	},
	/* CATCH */
	objc_debug_log("\t\tCatch block [%s]\n", class_getName(cl));
    if (object_getClass(exception) == ExceptionClass) {
		exceptionClass = ExceptionClass;
	}else if (object_getClass(exception) == OtherExceptionClass){
		exceptionClass = OtherExceptionClass;
	},
	/* FINALLY */
	{
		objc_debug_log("\t\tFinally block [%s]\n", class_getName(cl));
		wasInFinally = YES;
	});
	
	objc_assert(wasInTry, "Wasn't in the TRY block!\n");
	objc_assert(wasInFinally, "Wasn't in the FINALLY block!\n");
	objc_assert(exceptionClass == cl, "Caught wrong exception!\n");
}


void exception_test(void);
void exception_test(void){
	ExceptionClass = objc_allocateClassPair(Nil, "Exception", 0);
	objc_registerClassPair(ExceptionClass);
	
	OtherExceptionClass = objc_allocateClassPair(ExceptionClass, "OtherException", 0);
	objc_registerClassPair(OtherExceptionClass);
	
	
	run_exception_test_for_class(ExceptionClass);
	run_exception_test_for_class(OtherExceptionClass);
	
	
	objc_log("===================\n");
	objc_log("Passed exception tests.\n\n");
}


#include "../kernobjc/runtime.h"
#include "../os.h"
#include "../private.h"
#include "../KKObjects.h"

@interface ExceptionClass : KKObject
+(void)throw;
@end
@interface OtherExceptionClass : ExceptionClass
@end


@implementation ExceptionClass
+(void)throw{
	@throw [[self alloc] init];
}
@end
@implementation OtherExceptionClass
@end

static void run_exception_test_for_class(Class cl){
	BOOL was_in_try = NO;
	BOOL was_in_finally = NO;
	Class caught_for_class = Nil;
	void *rbp = NULL;
	void *rsp = NULL;
	__asm__("\t movq %%rbp, %0" : "=r"(rbp));
	__asm__("\t movq %%rsp, %0" : "=r"(rsp));
	
	objc_debug_log("Return address: %p\n", __builtin_return_address(0));
	
	@try {
		was_in_try = YES;
		id exc = [[cl alloc] init];
		@throw exc;
	}@catch (OtherExceptionClass *exception){
		//objc_debug_log("In OtherExceptionClass catch!\n");
		caught_for_class = [OtherExceptionClass class];
	}@catch (ExceptionClass *exception){
		//objc_debug_log("In ExceptionClass catch!\n");
		objc_debug_log("Return address in catch block: %p\n", (&cl)[-1]);
		caught_for_class = [ExceptionClass class];
	}@finally{
		//objc_debug_log("In finally!\n");
		objc_debug_log("Return address in finally: %p\n", (&cl)[-1]);
		was_in_finally = YES;
	}
	
	objc_debug_log("Return address after try-catch-finally: %p\n", (&cl)[-1]);
	objc_debug_log("RBP should be %p\n", rbp);
	objc_debug_log("RSP should be %p\n", rsp);
	
	objc_assert(was_in_try, "Wasn't in try!\n");
	objc_assert(was_in_try, "Wasn't in finally!\n");
	objc_assert(cl == caught_for_class, "Caught exception by the wrong handler:"
              " should be %p, is %p [%s]!\n", cl, caught_for_class,
              caught_for_class == Nil ? "null" : class_getName(caught_for_class));
}


void exception_test(void);
void exception_test(void){
	run_exception_test_for_class([ExceptionClass class]);
	run_exception_test_for_class([OtherExceptionClass class]);
	
	
	objc_log("===================\n");
	objc_log("Passed exception tests.\n\n");
}


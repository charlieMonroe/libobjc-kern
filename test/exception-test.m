#include "../kernobjc/runtime.h"
#include "../os.h"
#include "../private.h"
#include "../KKObjects.h"

@interface ExceptionClass : KKObject
@end
@interface OtherExceptionClass : ExceptionClass
@end


@implementation ExceptionClass
@end
@implementation OtherExceptionClass
@end


void __throwing_function(Class cl);
void __throwing_function(Class cl){
	@throw [[cl alloc] init];
}

static void run_exception_test_for_class(Class cl){
	BOOL was_in_try = NO;
	BOOL was_in_finally = NO;
	Class caught_for_class = Nil;
	@try {
		was_in_try = YES;
		__throwing_function(cl);
	}@catch (OtherExceptionClass *exception){
		objc_debug_log("In OtherExceptionClass catch!\n");
		caught_for_class = [OtherExceptionClass class];
	}@catch (ExceptionClass *exception){
		objc_debug_log("In ExceptionClass catch!\n");
		caught_for_class = [ExceptionClass class];
	}@finally{
		objc_debug_log("In finally!\n");
		was_in_finally = YES;
	}
	
	objc_assert(was_in_try, "Wasn't in try!\n");
	objc_assert(was_in_try, "Wasn't in finally!\n");
	objc_assert(cl == caught_for_class, "Caught exception by the wrong handler:"
				" %p [%s]!\n", caught_for_class, caught_for_class == Nil ?
				"null" : class_getName(caught_for_class));
}


void exception_test(void);
void exception_test(void){
	run_exception_test_for_class([ExceptionClass class]);
	run_exception_test_for_class([OtherExceptionClass class]);
	
	
	objc_log("===================\n");
	objc_log("Passed exception tests.\n\n");
}


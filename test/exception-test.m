#include "../kernobjc/runtime.h"
#include "../os.h"
#include "../private.h"
#include "../KKObjects.h"

@interface ExceptionClass : KKObject
+(void)throw;
@end
@interface OtherExceptionClass : ExceptionClass
@end
@interface ThirdExceptionClass : OtherExceptionClass
@end


@implementation ExceptionClass
+(void)throw{
	@throw [[self alloc] init];
}
@end
@implementation OtherExceptionClass
@end
@implementation ThirdExceptionClass
@end


/* The requires_exact_match flag says whether the class must be caught in its
 * own catch block, or can be caught by another one.
 */
static void run_exception_test_for_class(Class cl, BOOL requires_exact_match){
	BOOL was_in_try = NO;
	BOOL was_in_finally = NO;
	Class caught_for_class = Nil;
	
	objc_debug_log("--------------[ %s ]--------------\n", class_getName(cl));
	
	@try {
		objc_assert(!was_in_try, "Entering try for the second time!\n");
		was_in_try = YES;
		SEL throw_sel = sel_registerName("throw", "v16@0:8");
		if ([cl respondsToSelector:throw_sel]){
			[cl throw];
		}else{
			@throw [[cl alloc] init];
		}
	}@catch (OtherExceptionClass *exception){
		objc_assert(caught_for_class == NULL,
					"Entering catch for the second time!\n");
		caught_for_class = [OtherExceptionClass class];
	}@catch (ExceptionClass *exception){
		objc_assert(caught_for_class == NULL,
					"Entering catch for the second time!\n");
		caught_for_class = [ExceptionClass class];
	}@finally{
		objc_assert(!was_in_finally, "Entering finally for the second time!\n");
		was_in_finally = YES;
	}
	
	objc_assert(was_in_try, "Wasn't in try!\n");
	objc_assert(was_in_try, "Wasn't in finally!\n");
	if (requires_exact_match){
		objc_assert(cl == caught_for_class, "Caught exception by the wrong "
					"handler: should be %p, is %p [%s]!\n", cl,
					caught_for_class,
					caught_for_class == Nil ? "null" :
					class_getName(caught_for_class));
	}
}


void exception_test(void);
void exception_test(void){
	//run_exception_test_for_class([ExceptionClass class], YES);
	run_exception_test_for_class([OtherExceptionClass class], YES);
	run_exception_test_for_class([ThirdExceptionClass class], NO);
	
	BOOL caught = NO;
	@try{
		run_exception_test_for_class([KKObject class], NO);
	}@catch (...) {
		caught = YES;
	}
	
	objc_assert(caught, "Wasn't in the catch block!");
	
	objc_log("===================\n");
	objc_log("Passed exception tests.\n\n");
}


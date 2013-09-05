#include "../kernobjc/runtime.h"
#include "../os.h"
#include "../private.h"
#import "../kernobjc/KKObjects.h"

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


@interface ReturnCase : KKObject
+(void)performTest;
@end
@implementation ReturnCase

static BOOL flag;

+(void)_throwException{
	@try{
		return;
	}@finally {
		flag = YES;
	}
}
+(void)performTest{
	[self _throwException];
	
	objc_assert(flag == YES, "Flags hasn't been set!\n");
}

@end


/* The requires_exact_match flag says whether the class must be caught in its
 * own catch block, or can be caught by another one.
 *
 * The requires_to_be_caught flag indicates that the exception is required to be
 * caught. The requires_exact_match implicates this flag.
 */
static void run_exception_test_for_class(Class cl, BOOL requires_exact_match,
										 BOOL requires_to_be_caught){
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
	}else if (requires_to_be_caught){
		/* The exception possibly hasn't even been thrown? */
		objc_assert(cl != Nil, "Something went wrong.\n");
	}
}


/* Test from GNUstep ObjC Runtime */
BOOL finallyEntered = NO;
BOOL idRethrown = NO;
BOOL catchallRethrown = NO;
BOOL wrongMatch = NO;


static int throw(void)
{
	@throw [KKObject new];
}

static int finally(void)
{
	@try { throw(); }
	@finally  { finallyEntered = YES; }
	return 0;
}
static int rethrow_id(void)
{
	@try { finally(); }
	@catch(id x)
	{
		objc_assert(object_getClass(x) == [KKObject class], "Wrong class!\n");
		idRethrown = YES;
		@throw;
	}
	return 0;
}
static int rethrow_catchall(void)
{
	@try { rethrow_id(); }
	@catch(...)
	{
		catchallRethrown = YES;
		@throw;
	}
	return 0;
}

/* NestedExceptions test. */
static id a;

static void throw_nested(void){
	@throw a;
}

static void nested_exceptions_test(void){
	id e1 = [KKObject new];
	id e2 = [KKObject new];
	@try
	{
		a = e1;
		throw_nested();
	}
	@catch (id x)
	{
		objc_assert(x == e1, "Wrong catch.\n");
		@try {
			a = e2;
		}
		@catch (id y)
		{
			objc_assert(y == e2, "Wrong catch.\n");
		}
	}
	[e1 release];
	[e2 release];
}


void exception_test(void);
void exception_test(void){
	run_exception_test_for_class([ExceptionClass class], YES, YES);
	run_exception_test_for_class([OtherExceptionClass class], YES, YES);
	run_exception_test_for_class([ThirdExceptionClass class], NO, YES);
	
	BOOL caught = NO;
	@try{
		run_exception_test_for_class([KKObject class], NO, NO);
	}@catch (...) {
		caught = YES;
	}
	objc_assert(caught, "Wasn't in the catch block!");
	
	[ReturnCase performTest];
	
	
	@try
	{
		rethrow_catchall();
	}
	@catch (id x)
	{
		objc_assert(finallyEntered == YES, "Finally not entered!\n");
		objc_assert(idRethrown == YES, "id not rethrown!\n");
		objc_assert(catchallRethrown == YES, "catchall not rethrown!\n");
		objc_assert(wrongMatch == NO, "Wrong match!\n");
		objc_assert(object_getClass(x) == [KKObject class], "Wrong class!\n");
		[x release];
	}
	
	nested_exceptions_test();
	
	objc_log("===================\n");
	objc_log("Passed exception tests.\n\n");
}


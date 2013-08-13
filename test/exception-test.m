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


static void run_exception_test_for_class(Class cl){
	@try {
		// Something
	}@catch (ExceptionClass *exception){

	}@finally{

	}		
}


void exception_test(void);
void exception_test(void){
	run_exception_test_for_class([ExceptionClass class]);
	run_exception_test_for_class([OtherExceptionClass class]);
	
	
	objc_log("===================\n");
	objc_log("Passed exception tests.\n\n");
}


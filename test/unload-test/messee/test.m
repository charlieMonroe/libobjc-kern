#import "../../../os.h"
#import "../../../KKObjects.h"
#import "../../../kernobjc/runtime.h"

@interface MyClass : KKObject

-(void)sayHello;

@end


void run_test(void);
void run_test(void){
	MyClass *obj = [[MyClass alloc] init];
	
	BOOL exceptionWasCaught = NO;
	@try {
		[obj sayHello];
	}
	@catch (id exception) {
		exceptionWasCaught = YES;
		objc_log("Caught exception of class %s\n",
			 object_getClassName(exception));
	}
	
	objc_assert(exceptionWasCaught, "Exception was not thrown!\n");
	
	[obj release];
	
	objc_log("Passed unload test.\n");
}

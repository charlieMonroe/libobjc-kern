#import "../../../os.h"
#import "../../../KKObjects.h"
#import "../../../kernobjc/runtime.h"

@interface MyClass : KKObject

-(void)sayHello;

@end


void run_test(void);
void run_test(void){
	MyClass *obj = [[MyClass alloc] init];
	
	BOOL exception = NO;
	@try {
		[obj sayHello];
	}
	@catch (id exception) {
		exception = YES;
		objc_log("Caught exception of class %s\n",
			 object_getClassName(exception));
	}
	
	objc_assert(exception, "Exception was not thrown!\n");
	
	[obj release];
}

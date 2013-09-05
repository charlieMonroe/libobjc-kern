#import "../kernobjc/KKObjects.h"

BOOL didSomethingCrazy = NO;

@interface KKObject (Additions)

-(void)doSomethingCrazy;

@end

@implementation KKObject (Additions)

-(void)doSomethingCrazy{
	didSomethingCrazy = YES;
}

@end


void category_test(void);
void category_test(void)
{
	KKObject *obj = [KKObject new];
	
	[obj doSomethingCrazy];
	objc_assert(didSomethingCrazy, "Category method not invoked!\n")
	
	[obj release];
	
	
	objc_log("===================\n");
	objc_log("Passed category test.\n\n");
}


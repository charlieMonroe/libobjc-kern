#import "../myclass/MyClass.h"
#import "../../../os.h"

@interface MyClass (Additions)

-(void)sayHello;

@end

@implementation MyClass (Additions)

-(void)sayHello{
	/* This method shouldn't get called! */
	objc_log("Hello from MyClass!\n");
	objc_abort("Shouldn't be here!\n");
}

@end


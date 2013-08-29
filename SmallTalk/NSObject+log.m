#import "../Foundation/Foundation.h"

@implementation NSObject (log)
- (void) log
{
	NSLog(@"%@", [self description]);
}
- (void) printOn:(id)handle
{
	objc_log("%s\n", [[self description] UTF8String]);
}
- (void) ifNotNil: (id(^)(void))aBlock
{
	aBlock();
}
@end

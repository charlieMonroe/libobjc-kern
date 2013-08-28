
#import "NSObject.h"
#import "../kernobjc/message.h"

@implementation NSObject

-(id)description{
	return @"TODO";
}
-(id)performSelector:(SEL)selector withObject:(id)obj{
	return objc_msgSend(self, selector, obj);
}
-(id)self{
	return self;
}

@end

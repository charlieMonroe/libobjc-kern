
#import "NSObject.h"
#import "NSException.h"
#import "NSString.h"
#import "../kernobjc/runtime.h"

@implementation NSObject

+(void)subclassResponsibility:(SEL)selector{
	[[NSException exceptionWithName:@"NSAbstractClassException"
							 reason:[NSString stringWithUTF8String:sel_getName(selector)]
						   userInfo:nil] raise];
}

-(id)description{
	return @"TODO";
}
-(IMP)methodForSelector:(SEL)selector{
	return class_getMethodImplementation([self class], selector);
}
-(id)performSelector:(SEL)selector withObject:(id)obj{
	return objc_msgSend(self, selector, obj);
}
-(id)self{
	return self;
}

@end


#import "NSObject.h"
#import "NSException.h"
#import "NSString.h"
#import "NSArray.h"
#import "NSMethodSignature.h"
#import "../kernobjc/runtime.h"

@implementation NSObject

+(id)allocWithZone:(NSZone *)zone{
	return [self alloc];
}
+(id)className{
	return [NSString stringWithUTF8String:class_getName(self)];
}
+(NSArray *)directSubclasses{
	unsigned int numClasses = 0;
	Class *classes = objc_copyClassList(&numClasses);
	
	if (classes == NULL || numClasses == 0){
		return [NSArray array];
	}
	
	NSMutableArray *result = [NSMutableArray array];
	for (int i = 0; i < numClasses; i++) {
		Class c = classes[i];
		if (class_getSuperclass(c) == self){
			[result addObject:c];
	    }
	}
	
	objc_dealloc(classes, M_CLASS_TYPE);
	return result;
}
+(void)subclassResponsibility:(SEL)selector{
	[[NSException exceptionWithName:@"NSAbstractClassException"
							 reason:[NSString stringWithUTF8String:sel_getName(selector)]
						   userInfo:nil] raise];
}

-(id)className{
	return [NSString stringWithUTF8String:class_getName([self class])];
}
-(id)description{
	return [NSString stringWithFormat:@"<%@ %p>", [self className], self];
}
-(void)doesNotRecognizeSelector:(SEL)aSelector{
	objc_abort("Class %s does not recognize selector %s\n",
			   class_getName([self class]),
			   sel_getName(aSelector));
}
-(IMP)methodForSelector:(SEL)selector{
	return class_getMethodImplementation([self class], selector);
}
-(NSMethodSignature*)methodSignatureForSelector:(SEL)selector{
	return [NSMethodSignature signatureWithObjCTypes:sel_getTypes(selector)];
}
-(id)performSelector:(SEL)selector withObject:(id)obj{
	return objc_msgSend(self, selector, obj);
}
-(id)self{
	return self;
}
-(void)subclassResponsibility:(SEL)selector{
	[[NSException exceptionWithName:@"NSAbstractClassException"
							 reason:[NSString stringWithUTF8String:sel_getName(selector)]
						   userInfo:nil] raise];
}
-(id)value{
	return self;
}
@end

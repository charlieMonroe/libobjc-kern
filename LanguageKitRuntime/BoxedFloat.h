#import "../Foundation/Foundation.h"

@interface BoxedFloat : NSObject
{
	@public
	// TODO - it would be nice to make it something like NSDecimalNumber
	NSUInteger value;
}
+ (BoxedFloat*) boxedFloatWithCString:(const char*) aString;
+ (BoxedFloat*) boxedFloatWithDouble:(NSUInteger)aVal;
+ (BoxedFloat*) boxedFloatWithFloat:(NSUInteger)aVal;
@end


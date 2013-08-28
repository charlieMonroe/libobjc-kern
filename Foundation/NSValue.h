
#import "NSObject.h"
#import "NSTypes.h"

@class NSString;

typedef enum {
	NSValueTypeIntegral,
	NSValueTypePointer,
	NSValueTypeNonretainedObject,
	NSValueTypePoint,
	NSValueTypeRange,
	NSValueTypeRect,
	NSValueTypeSize
} NSValueType;

@interface NSValue : NSObject {
	union {
		long long			signedIntegralValue; /* Used in NSNumber */
		unsigned long long	unsignedIntegralValue; /* Used in NSNumber */
		const void			*pointer;
		NSPoint				point;
		NSRange				range;
		NSSize				size;
		NSRect				rect;
	} _data;
	NSValueType _type;
}

+(NSValue*)valueWithNonretainedObject:(id)anObject;
+(NSValue*)valueWithPoint:(NSPoint)point;
+(NSValue*)valueWithPointer:(const void*)pointer;
+(NSValue*)valueWithRange:(NSRange)range;
+(NSValue*)valueWithRect:(NSRect)rect;
+(NSValue*)valueWithSize:(NSSize)size;

-(void)getValue:(void*)value;

-(id)nonretainedObjectValue;
-(void*)pointerValue;
-(NSRange)rangeValue;
-(NSRect)rectValue;
-(NSSize)sizeValue;
-(NSPoint)pointValue;

@end

@interface NSNumber : NSValue

+(NSNumber*)numberWithBool:(BOOL)value;
+(NSNumber*)numberWithChar:(signed char)value;
+(NSNumber*)numberWithInt:(signed int)value;
+(NSNumber*)numberWithInteger:(NSInteger)value;
+(NSNumber*)numberWithLong:(signed long)value;
+(NSNumber*)numberWithLongLong:(signed long long)value;
+(NSNumber*)numberWithShort:(signed short)value;
+(NSNumber*)numberWithUnsignedChar:(unsigned char)value;
+(NSNumber*)numberWithUnsignedInt:(unsigned int)value;
+(NSNumber*)numberWithUnsignedInteger:(NSUInteger)value;
+(NSNumber*)numberWithUnsignedLong:(unsigned long)value;
+(NSNumber*)numberWithUnsignedLongLong:(unsigned long long)value;
+(NSNumber*)numberWithUnsignedShort:(unsigned short)value;

-(id)initWithBool:(BOOL)value;
-(id)initWithChar:(signed char)value;
-(id)initWithInt:(signed int)value;
-(id)initWithInteger:(NSInteger)value;
-(id)initWithLong:(signed long)value;
-(id)initWithLongLong:(signed long long)value;
-(id)initWithShort:(signed short)value;
-(id)initWithUnsignedChar:(unsigned char)value;
-(id)initWithUnsignedInt:(unsigned int)value;
-(id)initWithUnsignedInteger:(NSUInteger)value;
-(id)initWithUnsignedLong:(unsigned long)value;
-(id)initWithUnsignedLongLong:(unsigned long long)value;
-(id)initWithUnsignedShort:(unsigned short)value;

-(BOOL)boolValue;
-(signed char)charValue;
-(signed int)intValue;
-(NSInteger)integerValue;
-(signed long)longValue;
-(signed long long)longLongValue;
-(signed short)shortValue;
-(unsigned char)unsignedCharValue;
-(NSUInteger)unsignedIntegerValue;
-(unsigned int)unsignedIntValue;
-(unsigned long)unsignedLongValue;
-(unsigned long long)unsignedLongLongValue;
-(unsigned short)unsignedShortValue;


@end


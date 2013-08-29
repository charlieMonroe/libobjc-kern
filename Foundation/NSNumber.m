
#import "NSValue.h"
#import "NSException.h"

#import "../kernobjc/runtime.h"
#import "../private.h"

static BOOL kNSNumberUsesSmallObjects = NO;

#define NS_NUMBER_SMALL_MASK 0x1
#define NS_NUMBER_SMALL_SHIFT 0x3

#define NSSmallIntGetSignedValue(type)									\
				(type)((NSInteger)self >> NS_NUMBER_SMALL_SHIFT)

#define NSSmallIntGetUnsignedValue(type)								\
				(type)((NSUInteger)self >> NS_NUMBER_SMALL_SHIFT)

#define NSSmallIntGetValue(type, signed)								\
	signed ? NSSmallIntGetSignedValue(type) : NSSmallIntGetUnsignedValue(type)

#define NSNumberRedirectToSmallInt(signed)								\
	if (kNSNumberUsesSmallObjects) {									\
		return objc_msgSend((id)NSSmallIntClass, _cmd, value);			\
	}else{																\
		if (signed){													\
			return [[[self alloc] initWithLongLong:value] autorelease];	\
		}else{															\
			return [[[self alloc] initWithUnsignedLongLong:value] autorelease]; \
		}																\
	}

#define NSNumberInitWithValue(signed)									\
	if (signed){														\
		return [self initWithLongLong:value];							\
	}else{																\
		return [self initWithUnsignedLongLong:value];					\
	}

#define NSNumberReturnValue(type, signed)								\
	if (signed){														\
		return (type)[self longLongValue];								\
	}else{																\
		return (type)[self unsignedLongLongValue];						\
	}


static Class NSSmallIntClass;

@interface NSSmallInt : NSNumber

@end

@implementation NSSmallInt

+(void)load{
	kNSNumberUsesSmallObjects =
	objc_register_small_object_class(self, NS_NUMBER_SMALL_MASK);
	
	NSSmallIntClass = self;
}

+(NSNumber*)numberWithBool:(BOOL)value{
	return [self numberWithLong:value];
}
+(NSNumber*)numberWithChar:(signed char)value{
	return [self numberWithLong:value];
}
+(NSNumber*)numberWithInt:(signed int)value{
	return [self numberWithLong:value];
}
+(NSNumber*)numberWithLong:(signed long)value{
	return (id)((value << NS_NUMBER_SMALL_SHIFT) | NS_NUMBER_SMALL_MASK);
}
+(NSNumber*)numberWithShort:(signed short)value{
	return [self numberWithLong:value];
}
+(NSNumber*)numberWithUnsignedChar:(unsigned char)value{
	return [self numberWithLong:value];
}
+(NSNumber*)numberWithUnsignedInt:(unsigned int)value{
	return [self numberWithLong:value];
}
+(NSNumber*)numberWithUnsignedLong:(unsigned long)value{
	return (id)((value << NS_NUMBER_SMALL_SHIFT) | NS_NUMBER_SMALL_MASK);
}
+(NSNumber*)numberWithUnsignedShort:(unsigned short)value{
	return [self numberWithLong:value];
}


-(BOOL)boolValue{
	return NSSmallIntGetValue(BOOL, YES);
}
-(signed char)charValue{
	return NSSmallIntGetValue(signed char, YES);
}
-(signed int)intValue{
	return NSSmallIntGetValue(signed int, YES);
}
-(NSInteger)integerValue{
	return NSSmallIntGetValue(NSInteger, YES);
}
-(signed long)longValue{
	return NSSmallIntGetValue(signed long, YES);
}
-(signed long long)longLongValue{
	return NSSmallIntGetValue(signed long long, YES);
}
-(signed short)shortValue{
	return NSSmallIntGetValue(signed short, YES);
}
-(unsigned char)unsignedCharValue{
	return NSSmallIntGetValue(unsigned char, NO);
}
-(NSUInteger)unsignedIntegerValue{
	return NSSmallIntGetValue(NSUInteger, NO);
}
-(unsigned int)unsignedIntValue{
	return NSSmallIntGetValue(unsigned int, NO);
}
-(unsigned long)unsignedLongValue{
	return NSSmallIntGetValue(unsigned long, NO);
}
-(unsigned long long)unsignedLongLongValue{
	return NSSmallIntGetValue(unsigned long long, NO);
}
-(unsigned short)unsignedShortValue{
	return NSSmallIntGetValue(unsigned short, NO);
}

@end


@implementation NSNumber

+(NSNumber*)numberWithBool:(BOOL)value{
	NSNumberRedirectToSmallInt(YES);
}
+(NSNumber*)numberWithChar:(signed char)value{
	NSNumberRedirectToSmallInt(YES);
}
+(NSNumber*)numberWithInt:(signed int)value{
	NSNumberRedirectToSmallInt(YES);
}
+(NSNumber*)numberWithInteger:(NSInteger)value{
	return [[[self alloc] initWithInteger:value] autorelease];
}
+(NSNumber*)numberWithLong:(signed long)value{
	NSNumberRedirectToSmallInt(YES);
}
+(NSNumber*)numberWithLongLong:(signed long long)value{
	return [[[self alloc] initWithLongLong:value] autorelease];
}
+(NSNumber*)numberWithShort:(signed short)value{
	NSNumberRedirectToSmallInt(YES);
}
+(NSNumber*)numberWithUnsignedChar:(unsigned char)value{
	NSNumberRedirectToSmallInt(NO);
}
+(NSNumber*)numberWithUnsignedInt:(unsigned int)value{
	NSNumberRedirectToSmallInt(NO);
}
+(NSNumber*)numberWithUnsignedInteger:(NSUInteger)value{
	return [[[self alloc] initWithUnsignedInteger:value] autorelease];
}
+(NSNumber*)numberWithUnsignedLong:(unsigned long)value{
	NSNumberRedirectToSmallInt(NO);
}
+(NSNumber*)numberWithUnsignedLongLong:(unsigned long long)value{
	return [[[self alloc] initWithUnsignedLongLong:value] autorelease];
}
+(NSNumber*)numberWithUnsignedShort:(unsigned short)value{
	NSNumberRedirectToSmallInt(NO);
}

-(id)initWithBool:(BOOL)value{
	NSNumberInitWithValue(YES);
}
-(id)initWithChar:(signed char)value{
	NSNumberInitWithValue(YES);
}
-(id)initWithInt:(signed int)value{
	NSNumberInitWithValue(YES);
}
-(id)initWithInteger:(NSInteger)value{
	NSNumberInitWithValue(YES);
}
-(id)initWithLong:(signed long)value{
	NSNumberInitWithValue(YES);
}
-(id)initWithLongLong:(signed long long)value{
	if ((self = [super init]) != nil){
		_data.signedIntegralValue = value;
		_type = NSValueTypeIntegral;
	}
	return self;
}
-(id)initWithShort:(signed short)value{
	NSNumberInitWithValue(YES);
}
-(id)initWithUnsignedChar:(unsigned char)value{
	NSNumberInitWithValue(NO);
}
-(id)initWithUnsignedInt:(unsigned int)value{
	NSNumberInitWithValue(NO);
}
-(id)initWithUnsignedInteger:(NSUInteger)value{
	NSNumberInitWithValue(NO);
}
-(id)initWithUnsignedLong:(unsigned long)value{
	NSNumberInitWithValue(NO);
}
-(id)initWithUnsignedLongLong:(unsigned long long)value{
	if ((self = [super init]) != nil){
		_data.unsignedIntegralValue = value;
		_type = NSValueTypeIntegral;
	}
	return self;
}
-(id)initWithUnsignedShort:(unsigned short)value{
	NSNumberInitWithValue(NO);
}

-(const char *)objCType{
	/* Everything's long long */
	return @encode(unsigned long long);
}

-(BOOL)boolValue{
	NSNumberReturnValue(BOOL, YES);
}
-(signed char)charValue{
	NSNumberReturnValue(signed char, YES);
}
-(signed int)intValue{
	NSNumberReturnValue(signed int, YES);
}
-(NSInteger)integerValue{
	NSNumberReturnValue(NSInteger, YES);
}
-(signed long)longValue{
	NSNumberReturnValue(signed long, YES);
}
-(signed long long)longLongValue{
	if (_type != NSValueTypeIntegral){
		@throw [NSException exceptionWithName:NSInternalInconsistencyException
									   reason:@""
									 userInfo:nil];
	}
	return _data.signedIntegralValue;
}
-(signed short)shortValue{
	NSNumberReturnValue(signed short, YES);
}
-(unsigned char)unsignedCharValue{
	NSNumberReturnValue(unsigned char, NO);
}
-(NSUInteger)unsignedIntegerValue{
	NSNumberReturnValue(NSUInteger, NO);
}
-(unsigned int)unsignedIntValue{
	NSNumberReturnValue(unsigned int, NO);
}
-(unsigned long)unsignedLongValue{
	NSNumberReturnValue(unsigned long, NO);
}
-(unsigned long long)unsignedLongLongValue{
	if (_type != NSValueTypeIntegral){
		@throw [NSException exceptionWithName:NSInternalInconsistencyException
									   reason:@""
									 userInfo:nil];
	}
	return _data.unsignedIntegralValue;
}
-(unsigned short)unsignedShortValue{
	NSNumberReturnValue(unsigned short, NO);
}



@end

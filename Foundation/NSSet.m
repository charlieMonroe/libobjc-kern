
#import "NSSet.h"
#import "NSArray.h"
#import "NSException.h"

#ifdef _KERNEL
#include <machine/stdarg.h>
#else
#include <stdarg.h>
#endif

NSString *const NSSetNoStackMemoryException = @"NSSetNoStackMemoryException";


static inline void NSSetRaiseNoStackMemoryException(void){
	[[NSException exceptionWithName:NSSetNoStackMemoryException reason:@"" userInfo:nil] raise];
}

#define kNSSetMaxStackBufferSize 64

#define NSSetCreateStackBufferFromArgsAndPerformCode(bufferName, code)		\
			id bufferName[kNSSetMaxStackBufferSize];						\
			id obj;															\
			int count = 1;													\
			bufferName[0] = firstObject;									\
																			\
			va_list __ap;													\
			va_start(__ap, firstObject);									\
			while (count < kNSSetMaxStackBufferSize							\
				   && ((obj = va_arg(__ap, id)) != nil)){					\
				bufferName[count] = obj;									\
				++count;													\
			}																\
			va_end(__ap);													\
			if (count == kNSSetMaxStackBufferSize) {						\
				NSSetRaiseNoStackMemoryException();							\
			}																\
			code;


@implementation NSSet

+(id)set{
	return [[[self alloc] init] autorelease];
}
+(id)setWithArray:(NSArray*)objects{
	return [[[self alloc] initWithArray:objects] autorelease];
}
+(id)setWithObject:(id)anObject{
	return [[[self alloc] initWithObjects:&anObject count:1] autorelease];
}
+(id)setWithObjects:(id)firstObject, ...{
	NSSetCreateStackBufferFromArgsAndPerformCode(objects, {
		return [[[self alloc] initWithObjects:objects count:count] autorelease];
	});
}
+(id)setWithSet:(NSSet*)aSet{
	return [[[self alloc] initWithSet:aSet] autorelease];
}

-(NSArray*)allObjects{
	if (_array == nil){
		return [NSArray array];
	}
	return [[_array copy] autorelease];
}
-(id)anyObject{
	return [_array lastObject];
}
-(BOOL)containsObject:(id)anObject{
	return [_array containsObject:anObject];
}
-(NSUInteger)count{
	return [_array count];
}

-(void)dealloc{
	[_array release];
	
	[super dealloc];
}

-(id)init{
	if ((self = [super init]) != nil){
		_array = nil;
	}
	return self;
}
-(id)initWithArray:(NSArray*)other{
	return [self initWithObjects:[other getDirectObjectArray] count:[other count]];
}
-(id)initWithObjects:(id)firstObject, ...{
	NSSetCreateStackBufferFromArgsAndPerformCode(objects, {
		return [self initWithObjects:objects count:count];
	});
}
-(id)initWithObjects:(const id[])objects count:(NSUInteger)count{
	if (count == 0){
		return [self init];
	}
	
	if ((self = [super init]) != nil){
		_array = [[NSMutableArray alloc] initWithObjects:objects count:count];
	}
	return self;
}
-(id)initWithSet:(NSSet*)other{
	if ((self = [super init]) != nil){
		_array = [[NSMutableArray alloc] initWithObjects:[other->_array getDirectObjectArray]
												   count:[other count]];
	}
	return self;
}

@end


#import "NSArray.h"
#import "NSException.h"

#include "../os.h"

#ifdef _KERNEL
	#include <machine/stdarg.h>
#else
	#include <stdarg.h>
#endif

MALLOC_DEFINE(M_NSARRAY_TYPE, "NSArray_inner", "NSArray backend");

static NSString *const NSArrayOutOfBoundsException = @"NSArrayOutOfBoundsException";
static NSString *const NSArrayNoStackMemoryException = @"NSArrayNoStackMemoryException";


static inline void NSArrayRaiseNoStackMemoryException(void){
	[[NSException exceptionWithName:NSArrayNoStackMemoryException reason:@"" userInfo:nil] raise];
}

static inline void NSArrayRaiseOutOfBoundsException(void){
	[[NSException exceptionWithName:NSArrayOutOfBoundsException reason:@"" userInfo:nil] raise];
}

#define kNSArrayMaxStackBufferSize 64

#define NSArrayCreateStackBufferFromArgsAndPerformCode(bufferName, code)	\
			id bufferName[kNSArrayMaxStackBufferSize];						\
			id obj;															\
			int count = 0;													\
																			\
			va_list __ap;													\
			va_start(__ap, firstObject);									\
			while (count < kNSArrayMaxStackBufferSize						\
				   && ((obj = va_arg(__ap, id)) != nil)){					\
				buffer[count] = obj;										\
				++count;													\
			}																\
			va_end(__ap);													\
			if (count == kNSArrayMaxStackBufferSize) {						\
				NSArrayRaiseNoStackMemoryException();						\
			}																\
			code;


@implementation NSArray

+(id)array{
	// TODO singleton?
	return [[[self alloc] init] autorelease];
}
+(id)arrayWithArray:(NSArray*)array{
	return [[[self alloc] initWithArray:array] autorelease];
}

+(id)arrayWithObject:(id)anObject{
	return [[[self alloc] initWithObjects:&anObject count:1] autorelease];
}
+(id)arrayWithObjects:(id)firstObject, ...{
	NSArrayCreateStackBufferFromArgsAndPerformCode(buffer, {
		return [self arrayWithObjects:buffer count:count];
	});
}
+(id)arrayWithObjects:(const id[])objects count:(NSUInteger)count{
	return [[[self alloc] initWithObjects:objects count:count] autorelease];
}

-(NSArray*)arrayByAddingObject:(id)anObject{
	NSArray *array = [[NSArray alloc] init];
	
	array->_items = objc_alloc(sizeof(id) * (_count + 1), M_NSARRAY_TYPE);
	memcpy(array->_items, _items, sizeof(id) * _count);
	
	array->_items[_count] = anObject;
	array->_count = _count + 1;
	
	return [array autorelease];
}
-(NSArray*)arrayByAddingObjectsFromArray:(NSArray*)anotherArray{
	NSArray *array = [[NSArray alloc] init];
	
	array->_items = objc_alloc(sizeof(id) * (_count + anotherArray->_count),
							   M_NSARRAY_TYPE);
	
	memcpy(array->_items, _items, sizeof(id) * _count);
	memcpy((char*)array->_items + (sizeof(id) * _count),
		   anotherArray->_items,
		   sizeof(id) * anotherArray->_count);
	
	array->_count = _count + anotherArray->_count;
	
	return [array autorelease];
}
-(BOOL)containsObject:(id)anObject{
	for (NSUInteger i = 0; i < _count; ++i){
		if ([_items[i] isEqual:anObject]){
			return YES;
		}
	}
	return NO;
}

-(NSUInteger)count{
	return _count;
}

-(void)dealloc{
	if (_items != NULL){
		for (NSUInteger i = 0; i < _count; ++i){
			[_items[i] autorelease];
		}
		objc_dealloc(_items, M_NSARRAY_TYPE);
	}
	
	[super dealloc];
}

-(NSUInteger)indexOfObject:(id)anObject{
	return [self indexOfObject:anObject inRange:NSMakeRange(0, _count)];
}
-(NSUInteger)indexOfObject:(id)anObject inRange:(NSRange)aRange{
	if (aRange.location + aRange.length >= _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	for (NSUInteger i = aRange.location; i < aRange.location + aRange.length; ++i){
		if ([_items[i] isEqual:anObject]){
			return i;
		}
	}
	return NSNotFound;
}
-(NSUInteger)indexOfObjectIdenticalTo:(id)anObject{
	return [self indexOfObjectIdenticalTo:anObject inRange:NSMakeRange(0, _count)];
}
-(NSUInteger)indexOfObjectIdenticalTo:(id)anObject inRange:(NSRange)aRange{
	if (aRange.location + aRange.length >= _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	for (NSUInteger i = aRange.location; i < aRange.location + aRange.length; ++i){
		if (_items[i] == anObject){
			return i;
		}
	}
	return NSNotFound;
}

-(id)init{
	if ((self = [super init]) != nil){
		_items = NULL; // No need to allocate anything
		_count = 0;
	}
	return self;
}
-(id)initWithArray:(NSArray*)array{
	objc_assert(array != nil, "Creating array from nil!\n");
	return [self initWithObjects:array->_items count:array->_count];
}
-(id)initWithObjects:firstObject, ...{
	NSArrayCreateStackBufferFromArgsAndPerformCode(buffer, {
		return [self initWithObjects:buffer count:count];
	});
}

-(id)initWithObjects:(const id[])objects count:(NSUInteger)count{
	if (count == 0){
		return [self init];
	}
	
	if ((self = [super init]) != nil){
		size_t size = sizeof(id) * count;
		
		_items = objc_alloc(size, M_NSARRAY_TYPE);
		
		for (NSUInteger i = 0; i < count; ++i){
			_items[i] = [objects[i] retain];
		}
		
		_count = count;
	}
	return self;
}

-(id)lastObject{
	if ([self count] > 0){
		return _items[_count - 1];
	}
	return nil;
}
-(id)objectAtIndex:(NSUInteger)idx{
	if (idx >= _count){
		NSArrayRaiseOutOfBoundsException();
		__builtin_unreachable();
	}else{
		return _items[idx];
	}
}

@end

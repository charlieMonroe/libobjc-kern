
#import "NSArray.h"
#import "NSEnumerator.h"
#import "NSException.h"

#include "../os.h"

#ifdef _KERNEL
	#include <machine/stdarg.h>
#else
	#include <stdarg.h>
#endif

MALLOC_DEFINE(M_NSARRAY_TYPE, "NSArray_inner", "NSArray backend");

NSString *const NSArrayOutOfBoundsException = @"NSArrayOutOfBoundsException";
NSString *const NSArrayNoStackMemoryException = @"NSArrayNoStackMemoryException";


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


/// Swaps the two provided objects.
static inline void SwapObjects(id * o1, id * o2){
	id temp;
	
	temp = *o1;
	*o1 = *o2;
	*o2 = temp;
}

/**
 * Sorts the provided object array's sortRange according to sortDescriptor.
 */
// Quicksort algorithm copied from Wikipedia :-).
// Quicksort algorithm copied from GNUstep :-)
static void _GSQuickSort(id *objects, NSRange sortRange, id comparisonEntity,
								 void *context)
{
	if (sortRange.length > 1){
		id pivot = objects[sortRange.location];
		NSUInteger left = sortRange.location + 1;
		NSUInteger right = NSMaxRange(sortRange);
		
		while (left < right){
			NSComparisonResult (*comparator)(id, id, void *) =
					((NSComparisonResult (*)(id, id, void *))comparisonEntity);
			NSComparisonResult result = comparator(objects[left], pivot, context);
			if (result == NSOrderedDescending){
				SwapObjects(&objects[left], &objects[--right]);
            }else{
				left++;
            }
        }
		
		SwapObjects(&objects[--left], &objects[sortRange.location]);
		
		_GSQuickSort(objects,
					 NSMakeRange(sortRange.location, left - sortRange.location),
					 comparisonEntity, context);
		_GSQuickSort(objects,
					 NSMakeRange(right, NSMaxRange(sortRange) - right),
					 comparisonEntity, context);
    }
}


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

-(id)copy{
	return [[NSArray alloc] initWithArray:self];
}

-(NSUInteger)count{
	return _count;
}
-(NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
								 objects:(id [])stackbuf
								   count:(NSUInteger)len{
	/* For immutable arrays we can return the contents pointer directly. */
	NSUInteger count = _count - state->state;
	state->mutationsPtr = (unsigned long *)self;
	state->itemsPtr = _items + state->state;
	state->state += count;
	return count;
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

-(__unsafe_unretained const id *)getDirectObjectArray{
	return _items;
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

-(id)mutableCopy{
	return [[NSMutableArray alloc] initWithArray:self];
}

-(id)objectAtIndex:(NSUInteger)idx{
	if (idx >= _count){
		NSArrayRaiseOutOfBoundsException();
		__builtin_unreachable();
	}else{
		return _items[idx];
	}
}

-(NSEnumerator *)objectEnumerator{
	return [NSEnumerator enumeratorWithArray:self];
}

@end


@implementation NSMutableArray

+(id)arrayWithCapacity:(NSUInteger)numItems{
	return [[[self alloc] initWithCapacity:numItems] autorelease];
}

-(void)_growArray{
	if (_capacity == 0 || _items == NULL){
		/* Special case */
		NSUInteger newCapacity = 16;
		_items = objc_alloc(sizeof(id) * newCapacity, M_NSARRAY_TYPE);
		_capacity = newCapacity;
		return;
	}
	
	NSUInteger newCapacity = _capacity * 2;
	if (newCapacity % 16 != 0){
		/* Clear the bottom 4 bits -> dividable by 16 and add 16. */
		newCapacity = (((newCapacity >> 4) + 1) << 4);
	}
	
	_items = objc_realloc(_items, sizeof(id) * newCapacity, M_NSARRAY_TYPE);
	_capacity = newCapacity;
}

-(void)addObject:(id)anObject{
	if (_capacity == _count){
		/* No more space */
		[self _growArray];
	}
	
	_items[_count] = [anObject retain];
	++_count;
	++_version;
}
-(void)addObjectsFromArray:(NSArray *)otherArray{
	NSUInteger count = [otherArray count];
	for (NSUInteger i = 0; i < count; ++i){
		if (_capacity == _count){
			/* No more space */
			[self _growArray];
		}
		
		_items[_count] = [[otherArray objectAtIndex:i] retain];
		++_count;
		++_version;
	}
}
-(NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
								 objects:(id [])stackbuf
								   count:(NSUInteger)len{
	NSInteger count;
	
	/* This is cached in the caller at the start and compared at each
	 * iteration.   If it changes during the iteration then
	 * objc_enumerationMutation() will be called, throwing an exception.
	 */
	state->mutationsPtr = &_version;
	
#ifndef NS_MIN
	#define NS_MIN(A, B) ((A < B) ? A : B)
#endif
	
	count = NS_MIN(len, _count - state->state);
	
#undef NS_MIN
	
	/* If a mutation has occurred then it's possible that we are being asked to
	 * get objects from after the end of the array.  Don't pass negative values
	 * to memcpy.
	 */
	if (count > 0){
		memcpy(stackbuf, _items + state->state, count * sizeof(id));
		state->state += count;
    }else{
		count = 0;
    }
	state->itemsPtr = stackbuf;
	return count;
}
-(void)exchangeObjectAtIndex:(NSUInteger)i1 withObjectAtIndex:(NSUInteger)i2{
	if (i1 >= _count || i2 >= _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	id value1 = _items[i1];
	id value2 = _items[i2];
	
	_items[i1] = value2;
	_items[i2] = value1;
}
-(id)init{
	if ((self = [super init]) != nil){
		_capacity = 0;
	}
	return self;
}
-(id)initWithCapacity:(NSUInteger)numItems{
	if ((self = [super init]) != nil){
		_items = objc_alloc(sizeof(id) * numItems, M_NSARRAY_TYPE);
		_capacity = numItems;
	}
	return self;
}
-(id)initWithObjects:(const id[])objects count:(NSUInteger)count{
	if (count == 0){
		return [self init];
	}
	
	if ((self = [super initWithObjects:objects count:count]) != nil){
		_capacity = count;
	}
	return self;
}
-(void)insertObject:(id)anObject atIndex:(NSUInteger)index{
	objc_assert(anObject != nil, "Cannot insert nil!");
	
	if (_capacity == _count){
		[self _growArray];
	}
	
	if (index >= _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	/* Shift all beyond index by one. */
	id previous = [anObject retain];
	for (NSUInteger i = index; i < _count + 1; ++i){
		id p = _items[i];
		_items[i] = previous;
		previous = p;
	}
	++_count;
	++_version;
}
-(void)removeAllObjects{
	if (_items != NULL){
		for (NSUInteger i = 0; i < _count; ++i){
			[_items[i] autorelease];
		}
		
		_count = 0;
		_capacity = 0;
		
		objc_dealloc(_items, M_NSARRAY_TYPE);
		_items = NULL;
	}
	
}
-(void)removeLastObject{
	if (_count == 0){
		NSArrayRaiseOutOfBoundsException();
	}
	
	--_count;
	++_version;
	[_items[_count] autorelease];
}
-(void)removeObjectAtIndex:(NSUInteger)index{
	if (index >= _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	if (index == _count - 1){
		[self removeLastObject];
		return;
	}
	
	/* Shift all beyond index by one. */
	[_items[index] autorelease];
	for (NSUInteger i = index; i < _count; ++i){
		_items[i] = _items[i + 1];
	}
	
	--_count;
	++_version;
}
-(void)removeObject:(id)anObject{
	[self removeObject:anObject inRange:NSMakeRange(0, [self count])];
}
-(void)removeObject:(id)anObject inRange:(NSRange)aRange{
	if (aRange.location + aRange.length > _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	for (NSUInteger i = aRange.location; i < aRange.location + aRange.length; ++i){
		if ([_items[i] isEqual:anObject]){
			[self removeObjectAtIndex:i];
			return;
		}
	}
}
-(void)removeObjectIdenticalTo:(id)anObject{
	[self removeObjectIdenticalTo:anObject inRange:NSMakeRange(0, [self count])];
}
-(void)removeObjectIdenticalTo:(id)anObject inRange:(NSRange)aRange{
	if (aRange.location + aRange.length > _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	for (NSUInteger i = aRange.location; i < aRange.location + aRange.length; ++i){
		if (_items[i] == anObject){
			[self removeObjectAtIndex:i];
			return;
		}
	}
}
-(void)removeObjectsInArray:(NSArray*)otherArray{
	NSUInteger count = [otherArray count];
	for (NSUInteger i = 0; i < count; ++i){
		[self removeObject:[otherArray objectAtIndex:i]];
	}
}
-(void)removeObjectsInRange:(NSRange)aRange{
	for (NSUInteger i = 0; i < aRange.length; ++i){
		[self removeObjectAtIndex:aRange.location];
	}
}
-(void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)anObject{
	if (index > _count){
		NSArrayRaiseOutOfBoundsException();
	}
	
	[_items[index] autorelease];
	_items[index] = [anObject retain];
}
-(void)setArray:(NSArray *)otherArray{
	[self removeAllObjects];
	[self addObjectsFromArray:otherArray];
}
-(void)sortUsingFunction:(NSComparisonResult (*)(id, id, void *))compare context:(void *)context{
	_GSQuickSort(_items, NSMakeRange(0, _count), (id)compare, context);
}

@end

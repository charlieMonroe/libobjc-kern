//
//  NSMutableArray.m
//  libkernobjc_xcode
//
//  Created by Charlie Monroe on 8/27/13.
//  Copyright (c) 2013 Krystof Vasa. All rights reserved.
//

#import "NSMutableArray.h"
#import "NSMallocTypes.h"

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
	}
}
-(void)exchangeObjectAtIndex:(NSUInteger)i1 withObjectAtIndex:(NSUInteger)i2{
	// TODO check bounds
	
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
	
	// TODO bounds checking
	
	/* Shift all beyond index by one. */
	id previous = [anObject retain];
	for (NSUInteger i = index; i < _count + 1; ++i){
		id p = _items[i];
		_items[i] = previous;
		previous = p;
	}
	++_count;
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
	// TODO check for 0-length array
	--_count;
	[_items[_count] autorelease];
}
-(void)removeObjectAtIndex:(NSUInteger)index{
	// TODO bounds checking
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
}
-(void)removeObject:(id)anObject{
	[self removeObject:anObject inRange:NSMakeRange(0, [self count])];
}
-(void)removeObject:(id)anObject inRange:(NSRange)aRange{
	// TODO bounds checking
	
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
	// TODO bounds checking
	
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
	// TODO bounds checking
	[_items[index] autorelease];
	_items[index] = [anObject retain];
}
-(void)setArray:(NSArray *)otherArray{
	[self removeAllObjects];
	[self addObjectsFromArray:otherArray];
}

@end

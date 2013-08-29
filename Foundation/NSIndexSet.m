
#import "NSIndexSet.h"
#import "NSArray.h"
#import "NSValue.h"

@implementation NSIndexSet

+(id)indexSet{
	return [[[self alloc] init] autorelease];
}
+(id)indexSetWithIndex:(NSUInteger)anIndex{
	return [[[self alloc] initWithIndex:anIndex] autorelease];
}
+(id)indexSetWithIndexesInRange:(NSRange)aRange{
	return [[[self alloc] initWithIndexesInRange:aRange] autorelease];
}

-(BOOL)containsIndex:(NSUInteger)anIndex{
	if (_indexCount == 0){
		return NO;
	}
	
	if (_dataCount == 1){
		/* data is just one object */
		return NSLocationInRange(anIndex, [_data rangeValue]);
	}
	
	for (NSUInteger i = 0; i < [_data count]; ++i){
		NSValue *value = [_data objectAtIndex:i];
		if (NSLocationInRange(anIndex, [value rangeValue])){
			return YES;
		}
	}
	return NO;
	
}
-(BOOL)containsIndexesInRange:(NSRange)aRange{
	if (aRange.length > _indexCount){
		return NO;
	}
	if (aRange.length == 0){
		return YES;
	}
	
	for (NSUInteger i = aRange.location; i < NSMaxRange(aRange); ++i){
		if (![self containsIndex:i]){
			return NO;
		}
	}
	return YES;
}

-(NSUInteger)count{
	return _indexCount;
}
-(void)enumerateIndexesUsingBlock:(void (^)(NSUInteger idx, BOOL *stop))block{
	if (_dataCount == 0){
		return;
	}
	if (_dataCount == 1){
		NSRange r = [_data rangeValue];
		BOOL stop = NO;
		for (NSUInteger i = r.location; (stop == NO) && (i < r.location + r.length); ++i){
			block(i, &stop);
		}
		return;
	}
	
	BOOL stop = NO;
	for (NSValue *v in _data){
		NSRange r = [v rangeValue];
		for (NSUInteger i = r.location; (stop == NO) && (i < r.location + r.length); ++i){
			block(i, &stop);
		}
	}
}
-(void)dealloc{
	[_data release];
	
	[super dealloc];
}
-(NSUInteger)firstIndex{
	if (_dataCount == 0){
		return NSNotFound;
	}
	
	if (_dataCount == 1){
		return [_data rangeValue].location;
	}
	
	return [[_data objectAtIndex:0] rangeValue].location;
}

-(id)initWithIndex:(NSUInteger)anIndex{
	return [self initWithIndexesInRange:NSMakeRange(anIndex, 1)];
}
-(id)initWithIndexesInRange:(NSRange)aRange{
	if (aRange.length == 0){
		return [self init];
	}
	
	if ((self = [super init]) != nil){
		_data = [[NSValue valueWithRange:aRange] retain];
		_dataCount = 1;
		_indexCount = aRange.length;
	}
	return self;
}

-(BOOL)isEqualToIndexSet:(NSIndexSet*)aSet{
	if (_indexCount != [aSet count]){
		return NO;
	}
	
	if (_dataCount == 0){
		return YES;
	}
	
	if (_dataCount == 1){
		return NSEqualRanges([_data rangeValue], [aSet->_data rangeValue]);
	}
	
	for (NSUInteger i = 0; i < [_data count]; ++i){
		NSValue *v1 = [_data objectAtIndex:i];
		NSValue *v2 = [aSet->_data objectAtIndex:i];
		if (!NSEqualRanges([v1 rangeValue], [v2 rangeValue])){
			return NO;
		}
	}
	return YES;
}
-(BOOL)isEqual:(id)otherObj{
	if ([otherObj isKindOfClass:[NSIndexSet class]]){
		return [self isEqualToIndexSet:otherObj];
	}
	return NO;
}

-(NSUInteger)lastIndex{
	if (_dataCount == 0){
		return NSNotFound;
	}
	
	if (_dataCount == 1){
		return NSMaxRange([_data rangeValue]) - 1;
	}
	
	return NSMaxRange([[_data lastObject] rangeValue]) - 1;
}

@end

@implementation NSMutableIndexSet

-(void)addIndex:(NSUInteger)anIndex{
	if ([self containsIndex:anIndex]) {
		return;
	}
	
	if (_dataCount == 1){
		/* Maybe we're lucky */
		NSRange range = [_data rangeValue];
		if (anIndex == range.location - 1){
			--range.location;
			++range.length;
			[_data release];
			_data = [[NSValue valueWithRange:range] retain];
			++_indexCount;
			return;
		}else if (anIndex == NSMaxRange(range)){
			++range.length;
			[_data release];
			_data = [[NSValue valueWithRange:range] retain];
			++_indexCount;
			return;
		}
		
		/* Nope, need to turn it into an array */
		_data = [[NSArray alloc] initWithObjects:[_data autorelease], nil];
	}
	
	NSUInteger firstLargerIndex = NSNotFound;
	for (NSUInteger i = 0; i < [_data count]; ++i){
		NSValue *value = [_data objectAtIndex:i];
		NSRange range = [value rangeValue];
		
		/* Again, maybe we're lucky */
		if (anIndex == range.location - 1){
			--range.location;
			++range.length;
			
			NSValue *newValue = [NSValue valueWithRange:range];
			[_data replaceObjectAtIndex:i withObject:newValue];
			++_indexCount;
			return;
		}else if (anIndex == NSMaxRange(range)){
			++range.length;
			
			NSValue *newValue = [NSValue valueWithRange:range];
			[_data replaceObjectAtIndex:i withObject:newValue];
			++_indexCount;
			return;
		}
		
		/* Nope */
		if (range.location < anIndex){
			firstLargerIndex = i;
			break;
		}
	}
	
	NSValue *value = [NSValue valueWithRange:NSMakeRange(anIndex, 1)];
	if (firstLargerIndex == NSNotFound){
		[_data addObject:value];
	}else{
		[_data insertObject:value atIndex:firstLargerIndex];
	}
	++_indexCount;
}
-(void)addIndexes:(NSIndexSet*)aSet{
	if ([aSet count] == 0){
		return;
	}
	if ([aSet count] == 1){
		[self addIndexesInRange:[aSet->_data rangeValue]];
	}else{
		for (NSUInteger i = 0; i < [aSet->_data count]; ++i){
			NSValue *val = [aSet->_data objectAtIndex:i];
			[self addIndexesInRange:[val rangeValue]];
		}
	}
}
-(void)addIndexesInRange:(NSRange)aRange{
	for (NSUInteger i = aRange.location; i < NSMaxRange(aRange); ++i) {
		[self addIndex:i];
	}
}

-(void)removeAllIndexes{
	_dataCount = 0;
	_indexCount = 0;
	[_data release];
	_data = nil;
}
-(void)removeIndex:(NSUInteger)anIndex{
	if (![self containsIndex:anIndex]){
		return;
	}
	
	if (_dataCount == 1){
		/* Are we lucky? */
		NSRange range = [_data rangeValue];
		if (range.location == anIndex){
			++range.location;
			--range.length;
			
			[_data release];
			_data = [[NSValue valueWithRange:range] retain];
			--_indexCount;
			return;
		}else if (anIndex == NSMaxRange(range) - 1){
			--range.length;
			[_data release];
			_data = [[NSValue valueWithRange:range] retain];
			--_indexCount;
			return;
		}
		
		/* Need to split the range. */
		NSRange r1 = NSMakeRange(range.location, anIndex - range.location);
		NSRange r2 = NSMakeRange(anIndex + 1, NSMaxRange(range) - anIndex);
		
		[_data release];
		_data = [[NSArray alloc] initWithObjects:[NSValue valueWithRange:r1],
												 [NSValue valueWithRange:r2],
												 nil];
		
		_dataCount = 2;
		--_indexCount;
		return;
	}
	
	NSRange range;
	NSUInteger indexInData = NSNotFound;
	for (NSUInteger i = 0; i < [_data count]; ++i){
		NSValue *v = [_data objectAtIndex:i];
		range = [v rangeValue];
		if (NSLocationInRange(anIndex, range)){
			indexInData = i;
			break;
		}
	}
	
	objc_assert(indexInData != NSNotFound, "Couldn't find an index that was "
				"found a few moments ago!\n");
	
	/* Again, maybe we're lucky */
	if (anIndex == range.location){
		++range.location;
		--range.length;
		
		NSValue *newValue = [NSValue valueWithRange:range];
		[_data replaceObjectAtIndex:indexInData withObject:newValue];
		--_indexCount;
		return;
	}else if (anIndex == NSMaxRange(range) - 1){
		--range.length;
		
		NSValue *newValue = [NSValue valueWithRange:range];
		[_data replaceObjectAtIndex:indexInData withObject:newValue];
		--_indexCount;
		return;
	}
	
	/* Need to split the range. */
	NSRange r1 = NSMakeRange(range.location, anIndex - range.location);
	NSRange r2 = NSMakeRange(anIndex + 1, NSMaxRange(range) - anIndex);
	[_data replaceObjectAtIndex:indexInData withObject:[NSValue valueWithRange:r2]];
	[_data insertObject:[NSValue valueWithRange:r1] atIndex:indexInData];
	
	++_dataCount;
	--_indexCount;
	
}
-(void)removeIndexes:(NSIndexSet*)aSet{
	if ([aSet count] == 0){
		return;
	}
	if ([aSet count] == 1){
		[self removeIndexesInRange:[aSet->_data rangeValue]];
	}else{
		for (NSUInteger i = 0; i < [aSet->_data count]; ++i){
			NSValue *val = [aSet->_data objectAtIndex:i];
			[self removeIndexesInRange:[val rangeValue]];
		}
	}
}
-(void)removeIndexesInRange:(NSRange)aRange{
	for (NSUInteger i = aRange.location; i < NSMaxRange(aRange); ++i) {
		[self removeIndex:i];
	}
}

@end

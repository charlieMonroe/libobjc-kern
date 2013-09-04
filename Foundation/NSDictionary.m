

#import "NSDictionary.h"
#import "NSException.h"
#import "NSEnumerator.h"
#import "NSArray.h"

#ifdef _KERNEL
#include <machine/stdarg.h>
#else
#include <stdarg.h>
#endif

NSString *const NSDictionaryNoStackMemoryException = @"NSDictionaryNoStackMemoryException";
NSString *const NSDictionaryNilKeyException = @"NSDictionaryNilKeyException";
NSString *const NSDictionaryNilObjectException = @"NSDictionaryNilKeyException";

MALLOC_DEFINE(M_NSDICTIONARY_TYPE, "NSDictionary_inner", "NSDictionary backend");

static inline void NSDictionaryRaiseNilKeyException(void){
	[[NSException exceptionWithName:NSDictionaryNilKeyException reason:@"" userInfo:nil] raise];
}

static inline void NSDictionaryRaiseNilObjectException(void){
	[[NSException exceptionWithName:NSDictionaryNilObjectException reason:@"" userInfo:nil] raise];
}

static inline void NSDictionaryRaiseNoStackMemoryException(void){
	[[NSException exceptionWithName:NSDictionaryNoStackMemoryException reason:@"" userInfo:nil] raise];
}

#define kNSDictionaryMaxStackBufferSize 64

#define NSDictionaryCreateStackBufferFromArgsAndPerformCode(objectBufferName, keysBufferName, code)	\
			id objectBufferName[kNSDictionaryMaxStackBufferSize];			\
			id keysBufferName[kNSDictionaryMaxStackBufferSize];				\
			id obj;															\
			id key;															\
			int count = 1;													\
																			\
			va_list __ap;													\
			va_start(__ap, firstObject);									\
			objectBufferName[0] = firstObject;								\
			keysBufferName[0] = va_arg(__ap, id);							\
			while (count < kNSDictionaryMaxStackBufferSize){				\
				obj = va_arg(__ap, id);										\
				key = va_arg(__ap, id);										\
				if (obj == nil) break;										\
				if (key == nil) NSDictionaryRaiseNilKeyException();			\
				objectBufferName[count] = obj;								\
				keysBufferName[count] = key;								\
				++count;													\
			}																\
			va_end(__ap);													\
			if (count == kNSDictionaryMaxStackBufferSize) {					\
				NSDictionaryRaiseNoStackMemoryException();					\
			}																\
			code;

typedef struct _NSDictionaryPair {
	id key;
	id object;
} NSDictionaryPair;

struct _NSDictionaryBucket {
	union {
		NSDictionaryPair one;
		NSDictionaryPair *many;
	} data;
	unsigned short count;
};

@implementation NSDictionary

+(id)dictionary{
	return [[[self alloc] init] autorelease];
}
+(id)dictionaryWithDictionary:(NSDictionary*)otherDictionary{
	return [[[self alloc] initWithDictionary:otherDictionary] autorelease];
}
+(id)dictionaryWithObject:(id)object forKey:(id)key{
	return [[[self alloc] initWithObjects:&object forKeys:&key count:1] autorelease];
}
+(id)dictionaryWithObjects:(NSArray*)objects forKeys:(NSArray*)keys{
	return [[[self alloc] initWithObjects:objects forKeys:keys] autorelease];
}
+(id)dictionaryWithObjects:(const id[])objects forKeys:(const id[])keys count:(NSUInteger)count{
	return [[[self alloc] initWithObjects:objects forKeys:keys count:count] autorelease];
}
+(id)dictionaryWithObjectsAndKeys:(id)firstObject, ...{
	NSDictionaryCreateStackBufferFromArgsAndPerformCode(objects, keys, {
		return [self dictionaryWithObjects:objects forKeys:keys count:count];
	});
}

-(void)_releaseBuckets{
	if (_buckets != NULL){
		for (NSUInteger i = 0; i < _bucketCount; ++i){
			if (_buckets[i].count == 0){
				continue;
			}
			
			NSDictionaryBucket *bucket = &_buckets[i];
			if (bucket->count == 1){
				[bucket->data.one.key release];
				[bucket->data.one.object release];
			}else{
				for (NSUInteger o = 0; o < bucket->count; ++o){
					[bucket->data.many[o].key release];
					[bucket->data.many[o].object release];
				}
				objc_dealloc(bucket->data.many, M_NSDICTIONARY_TYPE);
			}
		}
		objc_dealloc(_buckets, M_NSDICTIONARY_TYPE);
	}
}
-(void)_insertObject:(id)obj forKey:(id)key{
	NSUInteger hash = [key hash];
	NSUInteger bucketIndex = hash % _bucketCount;
	NSDictionaryBucket *bucket = &_buckets[bucketIndex];
	
	if (bucket->count == 0){
		bucket->data.one.key = [key copy];
		bucket->data.one.object = [obj retain];
		bucket->count = 1;
	}else if (bucket->count == 1 && [key isEqual:bucket->data.one.key]){
		/* Just replace the obj. */
		[bucket->data.one.object release];
		bucket->data.one.object = [obj retain];
	}else if (bucket->count == 1){
		/* Need to make more space. */
		NSDictionaryPair *pairs = objc_alloc(sizeof(NSDictionaryPair) * 2,
											 M_NSDICTIONARY_TYPE);
		pairs[0].key = bucket->data.one.key;
		pairs[0].object = bucket->data.one.object;
		
		pairs[1].key = [key copy];
		pairs[1].object = [obj retain];
		
		bucket->data.many = pairs;
		bucket->count = 2;
	}else{
		bucket->data.many = objc_realloc(bucket->data.many,
										 sizeof(NSDictionaryPair) * bucket->count + 1,
										 M_NSDICTIONARY_TYPE);
		
		bucket->data.many[bucket->count].key = [key copy];
		bucket->data.many[bucket->count].object = [obj retain];
		
		++bucket->count;
	}
}

-(NSArray*)allKeys{
	NSMutableArray *keys = [[NSMutableArray alloc] initWithCapacity:_itemCount];
	for (NSUInteger i = 0; i < _bucketCount; ++i) {
		if (_buckets[i].count == 0){
			continue;
		}
		
		if (_buckets[i].count == 1) {
			[keys addObject:_buckets[i].data.one.key];
		}else{
			for (NSUInteger o = 0; o < _buckets[i].count; ++o){
				[keys addObject:_buckets[i].data.many[o].key];
			}
		}
	}
	return [keys autorelease];
}
-(NSArray*)allKeysForObject:(id)anObject{
	/* 
	 * We generally assume that the object is in the dictionary just once most
	 * of the time. 
	 */
	NSMutableArray *keys = [[NSMutableArray alloc] initWithCapacity:1];
	for (NSUInteger i = 0; i < _bucketCount; ++i) {
		NSDictionaryBucket *bucket = &_buckets[i];
		if (bucket->count == 0){
			continue;
		}
		
		if (bucket->count == 1 && [anObject isEqual:bucket->data.one.object]){
			[keys addObject:bucket->data.one.key];
		}else if (bucket->count > 1){
			for (NSUInteger o = 0; o < bucket->count; ++o){
				if ([anObject isEqual:bucket->data.many[o].object]){
					[keys addObject:bucket->data.many[o].key];
					break;
				}
			}
		}
	}
	return [keys autorelease];
}
-(NSArray*)allValues{
	NSMutableArray *values = [[NSMutableArray alloc] initWithCapacity:_itemCount];
	
	for (NSUInteger i = 0; i < _bucketCount; ++i) {
		NSDictionaryBucket *bucket = &_buckets[i];
		if (bucket->count == 0){
			continue;
		}
		if (bucket->count == 1){
			[values addObject:bucket->data.one.object];
		}else if (bucket->count > 1){
			for (NSUInteger o = 0; o < bucket->count; ++o){
				[values addObject:bucket->data.many[o].object];
			}
		}
	}
	
	return [values autorelease];
}

-(id)copy{
	return [[NSDictionary alloc] initWithDictionary:self];
}

-(NSUInteger)count{
	return _itemCount;
}

-(void)dealloc{
	[self _releaseBuckets];
	
	[super dealloc];
}

-(id)init{
	if ((self = [super init]) != nil){
		_buckets = NULL;
		_itemCount = 0;
		_bucketCount = 0;
	}
	return self;
}
-(id)initWithDictionary:(NSDictionary*)otherDictionary{
	return [self initWithDictionary:otherDictionary copyItems:NO];
}
-(id)initWithDictionary:(NSDictionary*)otherDictionary copyItems:(BOOL)shouldCopy{
	objc_assert(otherDictionary != nil, "Cannot init with a nil dictionary!\n");
	if ([otherDictionary count] == 0){
		return [self init];
	}
	
	if ((self = [super init]) != nil){
		_buckets = objc_zero_alloc(sizeof(NSDictionaryBucket) * otherDictionary->_bucketCount,
								   M_NSDICTIONARY_TYPE);
		for (NSUInteger i = 0; i < otherDictionary->_bucketCount; ++i){
			NSDictionaryBucket *otherBucket = &otherDictionary->_buckets[i];
			NSDictionaryBucket *bucket = &_buckets[i];
			
			if (otherBucket->count == 0){
				continue;
			}
			
			if (otherBucket->count == 1){
				bucket->data.one.key = [otherBucket->data.one.key copy];
				bucket->data.one.object = shouldCopy ? [otherBucket->data.one.object copy] : [otherBucket->data.one.object retain];
				bucket->count = 1;
			}else{
				bucket->data.many = objc_zero_alloc(sizeof(id) * otherBucket->count,
														   M_NSDICTIONARY_TYPE);
				for (NSUInteger o = 0; o < bucket->count; ++o){
					bucket->data.many[o].key = [otherBucket->data.many[o].key copy];
					bucket->data.many[o].object = shouldCopy ? [otherBucket->data.many[o].object copy] : [otherBucket->data.many[o].object retain];
				}
			}
		}
		
		_bucketCount = otherDictionary->_bucketCount;
		_itemCount = otherDictionary->_itemCount;
	}
	return self;
}
-(id)initWithObjects:(NSArray*)objects forKeys:(NSArray*)keys{
	objc_assert([objects count] == [keys count], "Creating a dictionary with "
				"different counts of keys and objects!\n");
	
	if ([objects count] == 0){
		return [self init];
	}
	
	return [self initWithObjects:[objects getDirectObjectArray]
						 forKeys:[keys getDirectObjectArray]
						   count:[objects count]];
}
-(id)initWithObjectsAndKeys:(id)firstObject, ...{
	NSDictionaryCreateStackBufferFromArgsAndPerformCode(objects, keys, {
		return [self initWithObjects:objects forKeys:keys count:count];
	});
}
-(id)initWithObjects:(const id[])objects forKeys:(const id[])keys count:(NSUInteger)count{
	if ((self = [super init]) != nil){
		/* The hardest part here is to guess the right number of buckets.
		 * We go for 4/3 of count, 16-aligned. */
		
		_bucketCount = (count * 4) / 3;
		if (_bucketCount % 16 != 0){
			_bucketCount = ((_bucketCount >> 4) + 1) << 4;
		}
		
		_buckets = objc_zero_alloc(sizeof(NSDictionaryBucket) * _bucketCount,
								   M_NSDICTIONARY_TYPE);
		
		for (NSUInteger i = 0; i < count; ++i){
			id key = keys[i];
			id obj = objects[i];
			
			[self _insertObject:obj forKey:key];
		}
		
		_itemCount = count;
	}
	return self;
}

-(BOOL)isEqual:(id)otherObj{
	if ([otherObj isKindOfClass:[NSDictionary class]]){
		return [self isEqualToDictionary:otherObj];
	}
	return NO;
}
-(BOOL)isEqualToDictionary:(NSDictionary*)other{
	if (other == self){
		return YES;
	}
	if (other == nil){
		return NO;
	}
	if ([other count] != [self count]){
		return NO;
	}
	
	for (NSUInteger i = 0; i < _bucketCount; ++i) {
		NSDictionaryBucket *bucket = &_buckets[i];
		if (bucket->count == 0){
			continue;
		}
		
		if (bucket->count == 1){
			NSDictionaryPair *pair = &bucket->data.one;
			if (![[other objectForKey:pair->key] isEqual:pair->object]){
				return NO;
			}
		}else if (bucket->count > 1){
			for (NSUInteger o = 0; o < bucket->count; ++o){
				NSDictionaryPair *pair = &bucket->data.many[o];
				if (![[other objectForKey:pair->key] isEqual:pair->object]){
					return NO;
				}
			}
		}
	}
	return YES;
}
-(NSEnumerator *)keyEnumerator{
	return [NSEnumerator enumeratorWithArray:[self allKeys]];
}
-(id)mutableCopy{
	return [[NSMutableDictionary alloc] initWithDictionary:self];
}

-(id)objectForKey:(id)aKey{
	if (aKey == nil){
		NSDictionaryRaiseNilKeyException();
	}
	
	if (_bucketCount == 0){
		return nil;
	}
	
	NSUInteger hash = [aKey hash];
	NSUInteger bucketIndex = hash % _bucketCount;
	NSDictionaryBucket *bucket = &_buckets[bucketIndex];
	
	if (bucket->count == 0){
		return nil;
	}
	
	if (bucket->count == 1 && [aKey isEqual:bucket->data.one.key]){
		return bucket->data.one.object;
	}else{
		for (NSUInteger o = 0; o < bucket->count; ++o){
			if ([aKey isEqual:bucket->data.many[o].key]){
				return bucket->data.many[o].object;
			}
		}
	}
	return nil;
}
-(NSEnumerator *)objectEnumerator{
	return [NSEnumerator enumeratorWithArray:[self allValues]];
}

@end


@implementation NSMutableDictionary

+(id)dictionaryWithCapacity:(NSUInteger)numItems{
	return [[[self alloc] initWithCapacity:numItems] autorelease];
}

-(id)initWithCapacity:(NSUInteger)numItems{
	if ((self = [super init]) != nil){
		if (numItems == 0){
			return self;
		}
		
		_bucketCount = numItems;
		if (_bucketCount % 16 != 0){
			_bucketCount = ((_bucketCount >> 4) + 1) << 4;
		}
		
		_buckets = objc_zero_alloc(sizeof(NSDictionaryBucket) * _bucketCount,
								   M_NSDICTIONARY_TYPE);
	}
	return self;
}
-(void)removeAllObjects{
	[self _releaseBuckets];
}
-(void)removeObjectForKey:(id)aKey{
	if (aKey == nil){
		NSDictionaryRaiseNilKeyException();
	}
	
	if (_bucketCount == 0){
		return;
	}
	
	NSUInteger hash = [aKey hash];
	NSUInteger bucketIndex = hash % _bucketCount;
	NSDictionaryBucket *bucket = &_buckets[bucketIndex];
	
	if (bucket->count == 0){
		return;
	}
	
	if (bucket->count == 1 && [aKey isEqual:bucket->data.one.key]){
		[bucket->data.one.key release];
		[bucket->data.one.object release];
		
		bucket->data.one.key = nil;
		bucket->data.one.object = nil;
		
		bucket->count = 0;
	}else{
		NSUInteger bucketIndex = NSNotFound;
		for (NSUInteger o = 0; o < bucket->count; ++o){
			if ([aKey isEqual:bucket->data.many[o].key]){
				bucketIndex = o;
				break;
			}
		}
		
		if (bucketIndex == NSNotFound){
			return;
		}
		
		[bucket->data.many[bucketIndex].key release];
		[bucket->data.many[bucketIndex].object release];
		
		for (NSUInteger o = bucketIndex; o < bucket->count - 1; ++o){
			bucket->data.many[o].key = bucket->data.many[o + 1].key;
			bucket->data.many[o].object = bucket->data.many[o + 1].object;
		}
		
		/* Two scenarios - just one obj left -> move to data.one, or just 
		 * realloc 
		 */
		
		if (bucket->count == 1){
			id key = bucket->data.many[0].key;
			id obj = bucket->data.many[0].object;
			
			objc_dealloc(bucket->data.many, M_NSDICTIONARY_TYPE);
			
			bucket->data.one.key = key;
			bucket->data.one.object = obj;
			
			bucket->count = 1;
		}else{
			--bucket->count;
			
			bucket->data.many = objc_realloc(bucket->data.many,
						 sizeof(NSDictionaryPair) * bucket->count,
						 M_NSDICTIONARY_TYPE);
		}
	}
	--_itemCount;
}
-(void)removeObjectsForKeys:(NSArray*)keyArray{
	for (NSUInteger i = 0; i < [keyArray count]; ++i){
		[self removeObjectForKey:[keyArray objectAtIndex:i]];
	}
}
-(void)setObject:(id)anObject forKey:(id)aKey{
	if (aKey == nil){
		NSDictionaryRaiseNilKeyException();
	}
	if (anObject == nil){
		NSDictionaryRaiseNilObjectException();
	}
	
	[self _insertObject:anObject forKey:aKey];
}

@end



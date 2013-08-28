
#import "NSObject.h"
#import "NSTypes.h"

@class NSArray;

typedef struct _NSDictionaryBucket NSDictionaryBucket;

@interface NSDictionary : NSObject {
	NSDictionaryBucket *_buckets;
	NSUInteger _bucketCount;
	NSUInteger _itemCount;
}

+(id)dictionary;
+(id)dictionaryWithDictionary:(NSDictionary*)otherDictionary;
+(id)dictionaryWithObject:(id)object forKey:(id)key;
+(id)dictionaryWithObjects:(NSArray*)objects forKeys:(NSArray*)keys;
+(id)dictionaryWithObjects:(const id[])objects forKeys:(const id[])keys count:(NSUInteger)count;
+(id)dictionaryWithObjectsAndKeys:(id)firstObject, ...;

-(NSArray*)allKeys;
-(NSArray*)allKeysForObject:(id)anObject;
-(NSArray*)allValues;

-(NSUInteger)count;

-(id)init;
-(id)initWithDictionary:(NSDictionary*)otherDictionary;
-(id)initWithDictionary:(NSDictionary*)other copyItems:(BOOL)shouldCopy;
-(id)initWithObjects:(NSArray*)objects forKeys:(NSArray*)keys;
-(id)initWithObjectsAndKeys:(id)firstObject, ...;
-(id)initWithObjects:(const id[])objects forKeys:(const id[])keys count:(NSUInteger)count;
-(BOOL)isEqualToDictionary:(NSDictionary*)other;

-(id)objectForKey:(id)aKey;


@end



@interface NSMutableDictionary : NSDictionary

+(id)dictionaryWithCapacity:(NSUInteger)numItems;

-(void)addEntriesFromDictionary:(NSDictionary*)otherDictionary;
-(id)initWithCapacity:(NSUInteger)numItems;
-(void)removeAllObjects;
-(void)removeObjectForKey:(id)aKey;
-(void)removeObjectsForKeys:(NSArray*)keyArray;
-(void)setObject:(id)anObject forKey:(id)aKey;

@end


extern NSString *const NSDictionaryNoStackMemoryException;
extern NSString *const NSDictionaryNilKeyException;
extern NSString *const NSDictionaryNilObjectException;


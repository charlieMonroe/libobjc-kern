
#import "NSObject.h"
#import "NSTypes.h"

@class NSArray, NSMutableArray, NSString;

@interface NSSet : NSObject {
	NSMutableArray *_array;
}

+(id)set;
+(id)setWithArray:(NSArray*)objects;
+(id)setWithObject:(id)anObject;
+(id)setWithObjects:(id)firstObject, ...;
+(id)setWithSet:(NSSet*)aSet;

-(NSArray*)allObjects;
-(id)anyObject;
-(BOOL)containsObject:(id)anObject;
-(NSUInteger)count;

-(id)init;
-(id)initWithArray:(NSArray*)other;
-(id)initWithObjects:(id)firstObject, ...;
-(id)initWithObjects:(const id[])objects count:(NSUInteger)count;
-(id)initWithSet:(NSSet*)other;

@end

@interface NSMutableSet : NSSet

+(id)setWithCapacity:(NSUInteger)numItems;

-(void)addObject:(id)anObject;
-(void)addObjectsFromArray:(NSArray*)array;
-(id)initWithCapacity:(NSUInteger)numItems;
-(void)intersectSet:(NSSet*)other;
-(void)minusSet:(NSSet*)other;
-(void)removeAllObjects;
-(void)removeObject:(id)anObject;
-(void)unionSet:(NSSet*)other;

@end

extern NSString *const NSSetNoStackMemoryException;

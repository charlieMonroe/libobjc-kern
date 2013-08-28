
#import "NSObject.h"
#import "NSTypes.h"

@class NSString;

@interface NSArray : NSObject {
@protected
	id			*_items;
	NSUInteger	_count;
}

+(id)array;
+(id)arrayWithArray:(NSArray*)array;

+(id)arrayWithObject:(id)anObject;
+(id)arrayWithObjects:(id)firstObject, ...;
+(id)arrayWithObjects:(const id[])objects count:(NSUInteger)count;

-(NSArray*)arrayByAddingObject:(id)anObject;
-(NSArray*)arrayByAddingObjectsFromArray:(NSArray*)anotherArray;
-(BOOL)containsObject:(id)anObject;

-(NSUInteger)count;

/* May be NULL! */
-(__unsafe_unretained const id*)getDirectObjectArray;

-(NSUInteger)indexOfObject:(id)anObject;
-(NSUInteger)indexOfObject:(id)anObject inRange:(NSRange)aRange;
-(NSUInteger)indexOfObjectIdenticalTo:(id)anObject;
-(NSUInteger)indexOfObjectIdenticalTo:(id)anObject inRange:(NSRange)aRange;

-(id)init;
-(id)initWithArray:(NSArray*)array;
-(id)initWithObjects:firstObject, ...;

-(id)initWithObjects:(const id[])objects count:(NSUInteger)count;

-(id)lastObject;

-(id)mutableCopy;

-(id)objectAtIndex:(NSUInteger)idx;

@end


@interface NSMutableArray : NSArray {
	NSUInteger	_capacity;
}

+(id)arrayWithCapacity:(NSUInteger)numItems;

-(void)addObject:(id)anObject;
-(void)addObjectsFromArray:(NSArray*)otherArray;

-(void)exchangeObjectAtIndex:(NSUInteger)i1 withObjectAtIndex:(NSUInteger)i2;

-(id)initWithCapacity:(NSUInteger)numItems;

-(void)insertObject:(id)anObject atIndex: (NSUInteger)index;

-(void)removeObjectAtIndex:(NSUInteger)index;

-(void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)anObject;

-(void)setArray:(NSArray*)otherArray;

-(void)removeAllObjects;
-(void)removeLastObject;
-(void)removeObject:(id)anObject;
-(void)removeObject:(id)anObject inRange:(NSRange)aRange;
-(void)removeObjectIdenticalTo:(id)anObject;
-(void)removeObjectIdenticalTo:(id)anObject inRange:(NSRange)aRange;
-(void)removeObjectsInArray:(NSArray*)otherArray;
-(void)removeObjectsInRange:(NSRange)aRange;

@end


extern NSString *const NSArrayOutOfBoundsException;
extern NSString *const NSArrayNoStackMemoryException;

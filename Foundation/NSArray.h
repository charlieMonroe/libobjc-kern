
#import "NSObject.h"
#import "NSTypes.h"

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
-(id)objectAtIndex:(NSUInteger)idx;

@end

extern NSString *const NSArrayOutOfBoundsException;
extern NSString *const NSArrayNoStackMemoryException;

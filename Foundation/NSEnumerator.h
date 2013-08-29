
#import "NSObject.h"
#import "NSTypes.h"
#import "NSArray.h"


@interface NSEnumerator : NSObject {
	id _representedObject;
	NSUInteger _index;
}

+(NSEnumerator*)enumeratorWithArray:(NSArray*)array;

-(NSArray*)allObjects;
-(NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState*)state
								 objects:(__unsafe_unretained id[])stackbuf
								   count:(NSUInteger)len;
-(id)nextObject;

@end

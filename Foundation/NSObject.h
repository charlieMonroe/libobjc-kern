

#import "KKObjects.h"

/*
 * Based on KKObject, NSObject implements what KKObject doesn't. KKObject is
 * meant to be a super-light version of NSObject.
 */
@interface NSObject : KKObject

-(id)description;
-(id)performSelector:(SEL)selector withObject:(id)obj;
-(id)self;

@end



#import "KKObjects.h"

@protocol NSObject

-(id)autorelease;
-(void)release;
-(id)retain;

@end

/*
 * Based on KKObject, NSObject implements what KKObject doesn't. KKObject is
 * meant to be a super-light version of NSObject.
 */
@interface NSObject : KKObject <NSObject>

+(void)subclassResponsibility:(SEL)selector;

-(id)description;
-(IMP)methodForSelector:(SEL)selector;
-(id)performSelector:(SEL)selector withObject:(id)obj;
-(id)self;

@end

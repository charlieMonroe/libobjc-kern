
#import "../KKObjects.h"
#import "NSTypes.h"

@protocol NSObject

-(id)autorelease;
-(void)release;
-(id)retain;

@end

@class NSArray, NSMethodSignature;

/*
 * Based on KKObject, NSObject implements what KKObject doesn't. KKObject is
 * meant to be a super-light version of NSObject.
 */
@interface NSObject : KKObject <NSObject>

+(id)allocWithZone:(NSZone*)zone;
+(id)className;
+(NSArray*)directSubclasses;
+(IMP)methodForSelector:(SEL)selector;
+(void)subclassResponsibility:(SEL)selector;

-(id)className;
-(id)description;
-(void)doesNotRecognizeSelector:(SEL)aSelector;
-(NSMethodSignature*)methodSignatureForSelector:(SEL)selector;
-(IMP)methodForSelector:(SEL)selector;
-(id)performSelector:(SEL)selector withObject:(id)obj;
-(void)subclassResponsibility:(SEL)selector;
-(id)self;

-(id)value; /* For the LanguageKit. Returns self, overridden by subclasses. */

@end

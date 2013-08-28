#import "NSObject.h"

/* Currently only returns a singleton that's common for all threads since the
 * only feature we want is getting the TLS data.
 */

@class NSMutableDictionary;

@interface NSThread : NSObject

+(NSThread*)currentThread;

-(NSMutableDictionary*)threadDictionary;

@end

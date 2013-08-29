
#import "NSObject.h"

@class NSString;

@interface NSException : NSObject

+(NSException*)exceptionWithName: (NSString*)name
						  reason: (NSString*)reason
						userInfo: (id)userInfo;

+(void)raise:(NSString*)name format:(NSString*)format, ...;
	

- (id) initWithName: (NSString*)name
			 reason: (NSString*)reason
		   userInfo: (id)userInfo;


-(void)raise;

@property (readwrite, retain) NSString  *name;
@property (readwrite, retain) NSString  *reason;
@property (readwrite, retain) id        userInfo;

@end

extern NSString *const NSInternalInconsistencyException;
extern NSString *const NSInvalidArgumentException;


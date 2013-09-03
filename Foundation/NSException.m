
#import "NSException.h"
#import "NSString.h"

#ifdef _KERNEL
	#include <machine/stdarg.h>
#else
	#include <stdarg.h>
#endif

NSString *const NSInternalInconsistencyException = @"NSInternalInconsistencyException";
NSString *const NSInvalidArgumentException = @"NSInvalidArgumentException";

@implementation NSException

+(NSException *)exceptionWithName:(NSString *)name reason:(NSString *)reason userInfo:(id)userInfo{
	objc_debug_log("Trowing exception with name %s reason %s\n", [name cString], [reason cString]);
	return [[[self alloc] initWithName:name reason:reason userInfo:userInfo] autorelease];
}

+(void)raise:(NSString*)name format:(NSString*)format, ...{
	va_list ap;
	va_start(ap, format);
	NSString *str = [[[NSString alloc] initWithFormat:format arguments: ap] autorelease];
	va_end(ap);
	
	[[NSException exceptionWithName:name reason:str userInfo:nil] raise];
}

-(id)initWithName:(NSString *)name reason:(NSString *)reason userInfo:(id)userInfo{
	if ((self = [super init]) != nil){
		[self setName:name];
		[self setReason:reason];
		[self setUserInfo:userInfo];
	}
	return self;
}

-(void)dealloc{
	[self setName:nil];
	[self setReason:nil];
	[self setUserInfo:nil];
	
	[super dealloc];
}

-(void)raise{
	@throw self;
}

@end

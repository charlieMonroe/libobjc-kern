
#import "NSThread.h"
#import "NSDictionary.h"

#import "../os.h"

static objc_tls_key kNSThreadTLSKey;

static void __NSThreadReleaseTLSData(void *data){
	[(id)data release];
}

@implementation NSThread

+(void)load{
	objc_register_tls(&kNSThreadTLSKey, __NSThreadReleaseTLSData);
}

+(NSThread *)currentThread{
	static NSThread *_sharedThread = nil;
	if (_sharedThread == nil){
		/* The worst thing that can happen here is that we create two instances. */
		_sharedThread = [[self alloc] init];
	}
	return _sharedThread;
}

-(NSMutableDictionary *)threadDictionary{
	NSMutableDictionary *dict = objc_get_tls_for_key(kNSThreadTLSKey);
	if (dict == nil){
		dict = [[NSMutableDictionary alloc] init];
		objc_set_tls_for_key(dict, kNSThreadTLSKey);
	}
	return dict;
}

@end

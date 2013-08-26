#import "../KKObjects.h"
#import "../os.h"

@interface LoadTest : KKObject

@end

BOOL gotLoaded = NO;

@implementation LoadTest

+(void)load{
	gotLoaded = YES;
}

@end


void load_test(void);
void load_test(void){
	objc_assert(gotLoaded, "Not loaded!\n");
	
	objc_log("===================\n");
	objc_log("Passed load test.\n\n");
}


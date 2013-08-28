#import "../KKObjects.h"
#import "../os.h"

@interface LoadTest : KKObject

@end

static BOOL gotLoaded = NO;
static BOOL gotInitialized = NO;

@implementation LoadTest

+(void)load{
	objc_assert(gotInitialized == NO, "Initialized before load!\n");
	gotLoaded = YES;
}

+(void)initialize{
	objc_assert(gotLoaded == YES, "Initializing before load!\n");
	gotInitialized = YES;
}

@end


void load_test(void);
void load_test(void){
	objc_assert(gotLoaded, "Not loaded!\n");
	objc_assert(gotInitialized, "Not initialized!\n");
	
	objc_log("===================\n");
	objc_log("Passed load test.\n\n");
}


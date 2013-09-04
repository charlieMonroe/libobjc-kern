
#import "../Foundation/Foundation.h"
#import "../utils.h"

void run_string_test(void);
void run_string_test(void){
	@autoreleasepool {
		NSString *constString = @"Hello World";
		const char *cString = [constString UTF8String];
		objc_assert(objc_strings_equal(cString, "Hello World"), "Wrong value\n");
		
		
		
		NSLog(@"Hello %02i %s!", 123, "world");
	}
}


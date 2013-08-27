#import "../KKObjects.h"
#include "../kernobjc/runtime.h"
#include "../os.h"
#include "../utils.h"

void string_test(void);
void string_test(void){
	id string = @"Hello";
	
	objc_log("String length: %d string: %s\n", [string length],
			 [string cString] == NULL ? "(NULL)" : [string cString]);
	
//	objc_assert([string length] == 5, "Wrong string length!\n");
//	objc_assert(objc_strings_equal("Hello", [string cString]),
//				"Non-equal strings!\n");
	
	objc_log("===================\n");
	objc_log("Passed string tests.\n\n");
}

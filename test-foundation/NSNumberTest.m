
#import "../Foundation/Foundation.h"
#import "../kernobjc/runtime.h"


void run_number_test(void);
void run_number_test(void){
	@autoreleasepool {
		NSNumber *number;
		
		number = [NSNumber numberWithBool:YES];
		objc_assert([number boolValue], "Wrong value\n");
		objc_assert([number unsignedIntegerValue] == 1, "Wrong value\n");
		
		number = [NSNumber numberWithChar:'u'];
		objc_assert([number boolValue], "Wrong value\n");
		objc_assert([number unsignedIntegerValue] == 'u', "Wrong value\n");
		
		number = [NSNumber numberWithInt:123];
		objc_assert([number boolValue], "Wrong value\n");
		objc_assert([number unsignedIntegerValue] == 123, "Wrong value\n");
		
		number = [NSNumber numberWithInteger:1234];
		objc_assert([number boolValue], "Wrong value\n");
		objc_assert([number unsignedIntegerValue] == 1234, "Wrong value\n");
		
		number = [NSNumber numberWithLong:12345];
		objc_assert([number boolValue], "Wrong value\n");
		objc_assert([number unsignedIntegerValue] == 12345, "Wrong value\n");
		
		number = [NSNumber numberWithLongLong:123456];
		objc_assert([number boolValue], "Wrong value\n");
		objc_assert([number unsignedIntegerValue] == 123456, "Wrong value\n");
		
		number = [NSNumber numberWithShort:21];
		objc_assert([number boolValue], "Wrong value\n");
		objc_assert([number unsignedIntegerValue] == 21, "Wrong value\n");
	}
}


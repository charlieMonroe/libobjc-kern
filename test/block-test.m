#import "../kernobjc/runtime.h"
#import "../os.h"

static BOOL global_block_test_ran = NO;

void block_test(void);
void block_test(void){
	__block BOOL local_block_test_ran = NO;
	
	void(^global_block_test)(void) = ^{
		global_block_test_ran = YES;
	};
	void(^local_block_test)(void) = ^{
		local_block_test_ran = YES;
	};
	
	global_block_test();
	local_block_test();
	
	objc_assert(global_block_test_ran, "Global test failed\n");
	objc_assert(local_block_test_ran, "Local test failed\n");
	
	
	objc_log("===================\n");
	objc_log("Passed compiler tests.\n\n");
}


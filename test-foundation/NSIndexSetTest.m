
#import "../Foundation/Foundation.h"


void run_indexset_test(void);
void run_indexset_test(void){
	@autoreleasepool {
		NSIndexSet *set = [NSIndexSet indexSet];
		objc_assert([set count] == 0, "Wrong count");
		objc_assert([set firstIndex] == NSNotFound, "Wrong value");
		objc_assert([set lastIndex] == NSNotFound, "Wrong value");
		objc_assert(![set containsIndex:123], "Wrong value");
		
		set = [NSIndexSet indexSetWithIndex:123];
		objc_assert([set count] == 1, "Wrong count");
		objc_assert([set firstIndex] == 123, "Wrong value");
		objc_assert([set lastIndex] == 123, "Wrong value");
		objc_assert([set containsIndex:123], "Wrong value");
		
		set = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(12, 10)];
		objc_assert([set count] == 10, "Wrong count");
		objc_assert([set firstIndex] == 12, "Wrong value");
		objc_assert([set lastIndex] == 21, "Wrong value");
		objc_assert([set containsIndex:15], "Wrong value");
		
		NSMutableIndexSet *mutableSet = [NSMutableIndexSet indexSet];
		objc_assert([mutableSet count] == 0, "Wrong count");
		objc_assert([mutableSet firstIndex] == NSNotFound, "Wrong value");
		objc_assert([mutableSet lastIndex] == NSNotFound, "Wrong value");
		objc_assert(![mutableSet containsIndex:123], "Wrong value");

		[mutableSet addIndex:123];
		objc_assert([mutableSet count] == 1, "Wrong count");
		objc_assert([mutableSet firstIndex] == 123, "Wrong value");
		objc_assert([mutableSet lastIndex] == 123, "Wrong value");
		objc_assert([mutableSet containsIndex:123], "Wrong value");
		
		[mutableSet addIndexesInRange:NSMakeRange(12, 10)];
		objc_assert([mutableSet count] == 11, "Wrong count");
		objc_assert([mutableSet firstIndex] == 12, "Wrong value");
		objc_assert([mutableSet lastIndex] == 123, "Wrong value");
		objc_assert([mutableSet containsIndex:123], "Wrong value");
		objc_assert([mutableSet containsIndex:15], "Wrong value");
		objc_assert(![mutableSet containsIndex:22], "Wrong value");
		
		[mutableSet removeIndex:123];
		objc_assert([mutableSet count] == 10, "Wrong count");
		objc_assert([mutableSet firstIndex] == 12, "Wrong value");
		objc_assert([mutableSet lastIndex] == 21, "Wrong value");
		objc_assert(![mutableSet containsIndex:123], "Wrong value");
		objc_assert([mutableSet containsIndex:15], "Wrong value");
		objc_assert(![mutableSet containsIndex:22], "Wrong value");
		
		[mutableSet removeIndexesInRange:NSMakeRange(15, 1)];
		objc_assert([mutableSet count] == 9, "Wrong count");
		objc_assert([mutableSet firstIndex] == 12, "Wrong value");
		objc_assert(![mutableSet containsIndex:22], "Wrong value");
		objc_assert([mutableSet lastIndex] == 21, "Wrong value");
		objc_assert(![mutableSet containsIndex:123], "Wrong value");
		objc_assert(![mutableSet containsIndex:15], "Wrong value");
	}
}

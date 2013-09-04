
#import "../Foundation/Foundation.h"


void run_value_test(void);
void run_value_test(void){
	@autoreleasepool {
		NSPoint p = NSMakePoint(1, 2);
		NSRange range = NSMakeRange(3, 4);
		NSRect r = NSMakeRect(5, 6, 7, 8);
		NSSize s = NSMakeSize(9, 10);
		
		NSValue *value = [NSValue valueWithNonretainedObject:(id)0x1234];
		objc_assert([value nonretainedObjectValue] == (id)0x1234, "Wrong value\n");
		
		value = [NSValue valueWithPoint:p];
		objc_assert(NSPointsEqual(p, [value pointValue]), "Wrong value\n");
		
		value = [NSValue valueWithPointer:(const void*)0x1234];
		objc_assert([value pointerValue] == (const void*)0x1234, "Wrong value\n");
		
		value = [NSValue valueWithRange:range];
		objc_assert(NSRangesEqual([value rangeValue], range), "Wrong value\n");
		
		value = [NSValue valueWithRect:r];
		objc_assert(NSRectsEqual([value rectValue], r), "Wrong value\n");
		
		value = [NSValue valueWithSize:s];
		objc_assert(NSSizesEqual([value sizeValue], s), "Wrong value\n");
		
	}
}


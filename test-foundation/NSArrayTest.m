
#import "../Foundation/Foundation.h"


void run_array_test(void);
void run_array_test(void){
	@autoreleasepool {
		KKObject *obj1 = [[[KKObject alloc] init] autorelease];
		KKObject *obj2 = [[[KKObject alloc] init] autorelease];
		KKObject *obj3 = [[[KKObject alloc] init] autorelease];
		
		NSArray *arr = [NSArray arrayWithObject:obj1];
		objc_assert([arr count] == 1, "Wrong object count\n");
		objc_assert([arr lastObject] == obj1, "Wrong object\n");
		
		arr = [NSArray arrayWithObjects:obj1, obj2, nil];
		objc_assert([arr count] == 2, "Wrong object count\n");
		objc_assert([arr lastObject] == obj2, "Wrong object\n");
		objc_assert([arr objectAtIndex:0] == obj1, "Wrong object\n");
		
		arr = [arr arrayByAddingObject:obj3];
		objc_assert([arr count] == 3, "Wrong object count\n");
		objc_assert([arr lastObject] == obj3, "Wrong object\n");
		
		objc_assert([arr containsObject:obj1], "Couldn't find object\n");
		objc_assert([arr containsObject:obj2], "Couldn't find object\n");
		objc_assert([arr containsObject:obj3], "Couldn't find object\n");
		
		objc_assert([arr indexOfObject:obj1] == 0, "Wrong index\n");
		objc_assert([arr indexOfObject:obj2] == 1, "Wrong index\n");
		objc_assert([arr indexOfObject:obj3] == 2, "Wrong index\n");
		
		KKObject *obj4 = [[[KKObject alloc] init] autorelease];
		NSMutableArray *mutableArr = [[arr mutableCopy] autorelease];
		[mutableArr addObject:obj4];
		
		objc_assert([mutableArr count] == 4, "Wrong object count\n");
		objc_assert([mutableArr lastObject] == obj4, "Wrong object\n");
		
		[mutableArr removeObject:obj2];
		
		objc_assert([mutableArr count] == 3, "Wrong object count\n");
		objc_assert(![mutableArr containsObject:obj2],
					"Contains object after removal\n");
	}
}

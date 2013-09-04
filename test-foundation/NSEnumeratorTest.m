
#import "../Foundation/Foundation.h"


void run_enumerator_test(void);
void run_enumerator_test(void){
	@autoreleasepool {
		id key1 = @"key1";
		id key2 = @"key2";
		id key3 = @"key3";
		
		KKObject *obj1 = [[[KKObject alloc] init] autorelease];
		KKObject *obj2 = [[[KKObject alloc] init] autorelease];
		KKObject *obj3 = [[[KKObject alloc] init] autorelease];
		
		NSArray *arr = [NSArray arrayWithObjects:obj1, obj2, obj3, nil];
		NSEnumerator *e = [arr objectEnumerator];
		
		objc_assert([e nextObject] == obj1, "Wrong object\n");
		objc_assert([e nextObject] == obj2, "Wrong object\n");
		objc_assert([e nextObject] == obj3, "Wrong object\n");
		objc_assert([e nextObject] == nil, "Wrong object\n");
		
		NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:obj1, key1,
							  obj2, key2, obj3, key3, nil];
		e = [dict keyEnumerator];
		
		/* Note that the NSDictionary calls *copy* on the keys, not retain,
		 * so technically, we cannot use pointer comparison!
		 */
		objc_assert([[e nextObject] isEqual:key1], "Wrong object\n");
		objc_assert([[e nextObject] isEqual:key2], "Wrong object\n");
		objc_assert([[e nextObject] isEqual:key3], "Wrong object\n");
		objc_assert([e nextObject] == nil, "Wrong object\n");
		
		e = [dict objectEnumerator];
		objc_assert([e nextObject] == obj1, "Wrong object\n");
		objc_assert([e nextObject] == obj2, "Wrong object\n");
		objc_assert([e nextObject] == obj3, "Wrong object\n");
		objc_assert([e nextObject] == nil, "Wrong object\n");
	}
}

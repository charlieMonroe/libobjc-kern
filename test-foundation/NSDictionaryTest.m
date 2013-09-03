
#import "../Foundation/Foundation.h"


void run_dictionary_test(void);
void run_dictionary_test(void){
	@autoreleasepool {
		id key1 = @"key1";
		id key2 = @"key2";
		id key3 = @"key3";
		
		KKObject *obj1 = [[[KKObject alloc] init] autorelease];
		KKObject *obj2 = [[[KKObject alloc] init] autorelease];
		KKObject *obj3 = [[[KKObject alloc] init] autorelease];
		
		NSDictionary *dictionary = [NSDictionary dictionaryWithObject:obj1 forKey:key1];
		objc_assert([dictionary count] == 1, "Wrong object count\n");
		objc_assert([dictionary objectForKey:key1] == obj1, "Wrong object\n");
		objc_assert([dictionary objectForKey:key2] == nil, "Wrong object\n");
		
		dictionary = [NSDictionary dictionaryWithObjectsAndKeys:obj1, key1, obj2, key2, obj3, key3, nil];
		objc_assert([dictionary count] == 3, "Wrong object count\n");
		objc_assert([dictionary objectForKey:key1] == obj1, "Wrong object\n");
		objc_assert([dictionary objectForKey:key2] == obj2, "Wrong object\n");
		objc_assert([dictionary objectForKey:key3] == obj3, "Wrong object\n");
				
		KKObject *obj4 = [[[KKObject alloc] init] autorelease];
		
		NSMutableDictionary *mutableDict = [[dictionary mutableCopy] autorelease];
		
		objc_assert([mutableDict count] == 3, "Wrong object count\n");
		objc_assert([mutableDict objectForKey:key1] == obj1, "Wrong object\n");
		objc_assert([mutableDict objectForKey:key2] == obj2, "Wrong object\n");
		objc_assert([mutableDict objectForKey:key3] == obj3, "Wrong object\n");
		
		[mutableDict setObject:obj4 forKey:key1];
		objc_assert([mutableDict objectForKey:key1] == obj4, "Wrong object\n");
		
		[mutableDict removeObjectForKey:key1];
		objc_assert([mutableDict count] == 2, "Wrong object count\n");
		objc_assert([mutableDict objectForKey:key1] == nil, "Wrong object\n");
		objc_assert([mutableDict objectForKey:key2] == obj2, "Wrong object\n");
		objc_assert([mutableDict objectForKey:key3] == obj3, "Wrong object\n");
		
		[mutableDict removeObjectForKey:key1];
		[mutableDict removeObjectForKey:key2];
		[mutableDict removeObjectForKey:key3];
		
		objc_assert([mutableDict count] == 0, "Wrong object count\n");
		objc_assert([mutableDict objectForKey:key1] == nil, "Wrong object\n");
		objc_assert([mutableDict objectForKey:key2] == nil, "Wrong object\n");
		objc_assert([mutableDict objectForKey:key3] == nil, "Wrong object\n");
	}
}

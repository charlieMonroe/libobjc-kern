#include "../kernobjc/runtime.h"
#include "../KKObjects.h"
#include "../os.h"
#include "../utils.h"

#ifdef __has_attribute
#if __has_attribute(objc_root_class)
__attribute__((objc_root_class))
#endif
#endif
@interface helloclass {
	@private int varName;
}
@property (readwrite,assign) int propName;
@end

@implementation helloclass
@synthesize propName = varName;
+ (id)class { return self; }
@end

void property_test(void);
void property_test(void) {
	unsigned int outCount;
	objc_property_t *properties = class_copyPropertyList([helloclass class], &outCount);
	objc_assert(outCount == 1, "Wrong count");
	objc_property_t property = properties[0];
	objc_assert(objc_strings_equal(property_getName(property), "propName"),
				"Wrong name");
	objc_assert(objc_strings_equal(property_getAttributes(property), "Ti,VvarName"),
				"Wrong attributes");
	
	objc_dealloc(properties, M_PROPERTY_TYPE);
	
	Method* methods = class_copyMethodList([helloclass class], &outCount);
	objc_assert(outCount == 2, "Wrong method count");
	objc_dealloc(methods, M_METHOD_TYPE);

	objc_property_attribute_t a = { "V", "varName" };
	objc_assert(class_addProperty([helloclass class], "propName2", &a, 1),
				"Couldn't add property");
	
	properties = class_copyPropertyList([helloclass class], &outCount);
	objc_assert(outCount == 2, "Wrong property count");
	int found = 0;
	for (int i=0 ; i<2 ; i++)
	{
		property = properties[i];
		objc_debug_log("Name: %s\n", property_getName(property));
		objc_debug_log("Attrs: %s\n", property_getAttributes(property));
		if (objc_strings_equal(property_getName(property), "propName2"))
		{
			objc_assert(objc_strings_equal(property_getAttributes(property), "VvarName"),
						"Wrong type");
			found++;
		}
	}
	objc_assert(found == 1, "Couldn't find the added property!");
    
    objc_log("===================\n");
	objc_log("Passed property test.\n\n");
}



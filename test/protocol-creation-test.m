#import "../kernobjc/runtime.h"
#import "../os.h"
#import "../utils.h"

@protocol Test2 @end

void protocol_creation_test(void);
void protocol_creation_test(void)
{
	Protocol *p = objc_allocateProtocol("Test");
	
	/* Mustn't use @selector since no such selector exists. */
	protocol_addMethodDescription(p, sel_registerName("someMethod", "v@:"), "v@:", YES, NO);
	objc_assert(objc_getProtocol("Test2") != NULL, "Couldn't get protocol Test2\n");
	protocol_addProtocol(p, objc_getProtocol("Test2"));
	objc_property_attribute_t attrs[] = { {"T", "@" }, {"V", "foo"} };
	protocol_addProperty(p, "foo", attrs, 2, YES, YES);
	objc_registerProtocol(p);
	Protocol *p1 = objc_getProtocol("Test");
	objc_assert(p == p1, "Protocols not equal\n");
	
	/* Here we can use @selector since we've already registered the selector before. */
	struct objc_method_description d = protocol_getMethodDescription(p1, @selector(someMethod), YES, NO);
	
	objc_assert(objc_strings_equal(sel_getName(d.selector), "someMethod"), "Names not equal!\n");
	objc_assert(objc_strings_equal((d.selector_types), "v@:"), "Types not equal!\n");
	objc_assert(protocol_conformsToProtocol(p1, objc_getProtocol("Test2")), "Protocols do not conform!\n");
	
	unsigned int count;
	objc_property_t *props = protocol_copyPropertyList(p1, &count);
	
	objc_assert(count == 1, "Count not zero!\n");
	objc_assert(objc_strings_equal("T@,Vfoo", property_getAttributes(*props)), "Property attributes do not equal!\n");
	
	objc_dealloc(props, M_PROPERTY_TYPE);
	
	objc_log("===================\n");
	objc_log("Passed protocol creation test.\n\n");
}

#import "../kernobjc/runtime.h"
#import "../os.h"
#import "../utils.h"


#ifdef __has_attribute
#if __has_attribute(objc_root_class)
__attribute__((objc_root_class))
#endif
#endif
@interface Foo
@property (getter=bar, setter=setBar:, nonatomic, copy) id foo;
@end
@interface Foo(Bar)
-(id)bar;
-(void)setBar:(id)b;
@end 
@implementation Foo
@synthesize  foo;
@end

#define assert(x) objc_assert(x, "Wrong value at line %d", __LINE__)

void property_introspection_test1(void);
void property_introspection_test1(void){
	objc_property_t p = class_getProperty(objc_getClass("Foo"), "foo");
	unsigned int count;
	objc_property_attribute_t *l = property_copyAttributeList(p, &count);
	for (unsigned int i=0 ; i<count ; i++)
	{
		switch (l[i].name[0])
		{
			case 'T': assert(objc_strings_equal(l[i].value, "@")); break;
			case 'C': assert(objc_strings_equal(l[i].value, "")); break;
			case 'N': assert(objc_strings_equal(l[i].value, "")); break;
			case 'G': assert(objc_strings_equal(l[i].value, "bar")); break;
			case 'S': assert(objc_strings_equal(l[i].value, "setBar:")); break;
			case 'B': assert(objc_strings_equal(l[i].value, "foo")); break;
		}
	}
	objc_assert(0 == property_copyAttributeList(0, &count), "Couldn't copy attlist");
}


#define OBJC_HAS_CATEGORIES_EXTENSION 1
#include "testing.h"

GENERATE_TEST(category, "MySubclass", {}, DISPATCH_ITERATIONS, {
	SEL selector = NULL;
	IMP impl = NULL;
	OBJC_GET_IMP((id)instance, "incrementViaCategoryMethod", selector, impl);
	impl((id)instance, selector);
}, (*((int*)(objc_object_get_variable((id)instance, objc_class_get_ivar(objc_class_for_name("MySubclass"), "i")))) == DISPATCH_ITERATIONS))

int main(int argc, const char * argv[]){
	register_classes();
	perform_tests(category_test);
	return 0;
}

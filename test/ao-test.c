
#define OBJC_HAS_AO_EXTENSION 1

#include "testing.h"

GENERATE_TEST(ao, "MySubclass", {}, DISPATCH_ITERATIONS, {
	SEL selector = NULL;
	IMP impl = NULL;
	OBJC_GET_IMP((id)instance, "incrementViaAO", selector, impl);
	impl((id)instance, selector);
}, ((int)(objc_object_get_associated_object((id)instance, objc_selector_register("incrementViaAO"))) == DISPATCH_ITERATIONS))

int main(int argc, const char * argv[]){
	register_classes();
	perform_tests(ao_test);
	return 0;
}

#include "testing.h"

GENERATE_TEST(super_dispatch, "MySubclass", {}, DISPATCH_ITERATIONS, {
	SEL selector = NULL;
	IMP impl = NULL;
	OBJC_GET_IMP((id)instance, "increment", selector, impl);
	impl((id)instance, selector);
}, (*((int*)(objc_object_get_variable((id)instance, objc_class_get_ivar(objc_getClass("MySubclass"), "i")))) == 2 * DISPATCH_ITERATIONS))

int main(int argc, const char * argv[]){
	register_classes();
	
	{
		Method m = (Method)&_I_MySubclass_increment_mp_;
		m->selector = sel_registerName(_I_MySubclass_increment_mp_.selector_name);
		objc_class_add_instance_method(objc_getClass("MySubclass"), m);
	}
	
	perform_tests(super_dispatch_test);
	return 0;
}

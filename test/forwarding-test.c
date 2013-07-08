/**
 * WARNING: This test doesn't return times,
 * but a proxy / normal call ratio.
 */

#include "testing.h"

static BOOL reachedLastIteration = NO;

static void _I_NewClass_unknownSelector(id self, SEL _cmd, int i){
	if (i == (DISPATCH_ITERATIONS - 1)){
		reachedLastIteration = YES;
	}
}

static clock_t forwarding_test(void){
	id new_class_instance;
	MyClass *my_class_instance;
	clock_t c1, c2;
	int i;
	
	new_class_instance = objc_class_create_instance(objc_class_for_name("NewClass"));
	my_class_instance = (MyClass*)objc_class_create_instance(objc_class_for_name("MyClass"));
	my_class_instance->proxyObject = new_class_instance;
	
	c1 = clock();
	for (i = 0; i < DISPATCH_ITERATIONS; ++i){
		SEL selector = NULL;
		IMP impl = NULL;
		OBJC_GET_IMP((id)my_class_instance, "unknownSelector:", selector, impl);
		impl((id)my_class_instance, selector, i);
	}
	c2 = clock();
	
	objc_object_deallocate((id)my_class_instance);
	objc_object_deallocate(new_class_instance);
	
	if (!reachedLastIteration){
		printf("Haven't reached last iteration!\n");
		objc_abort("");
		return 0;
	}
	
	return (c2 - c1);
}

int main(int argc, const char * argv[]){
	register_classes();
	
	{
		Class completely_new_class = objc_class_create(Nil, "NewClass");
		SEL unknown_selector = objc_selector_register("unknownSelector:");
		objc_class_add_ivar(completely_new_class, "isa", sizeof(Class), __alignof(Class), "#");
		objc_class_add_instance_method(completely_new_class, objc_method_create(unknown_selector, "v@:i", (IMP)_I_NewClass_unknownSelector));
		objc_class_finish(completely_new_class);	
	}
	
	perform_tests(forwarding_test);
	return 0;
}


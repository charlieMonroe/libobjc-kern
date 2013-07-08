
#define OBJC_FORWARDING_TEST 1
#define NONFOUNDATION_FORWARDING 1
#include "testing-cocoa.h"
#include <stdlib.h>

clock_t forwarding_test(void){
	clock_t c1, c2;
	id new_class_instance = class_createInstance(objc_getClass("NewClass"), 0);
	id my_class_instance = class_createInstance(objc_getClass("MyClass"), 0);
	((MyClass*)my_class_instance)->proxyObject = new_class_instance;
	
	if (new_class_instance == nil || my_class_instance == nil){
		printf("new_class_instance or my_class_instance is nil!\n");
		abort();
		return 0;
	}
	
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		@autoreleasepool {
			OBJC_MSG_SEND_DEBUG(my_class_instance, unknownSelector);
		}
	}
	
	c2 = clock();
	
	object_dispose(new_class_instance);
	object_dispose(my_class_instance);
	
	return (c2 - c1);
}

int main(int argc, const char *argv[]){
	perform_tests(forwarding_test);
	return 0;
}

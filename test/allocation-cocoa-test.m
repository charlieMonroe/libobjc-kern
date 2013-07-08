
#include "testing-cocoa.h"


clock_t allocation_test(void){
	clock_t c1, c2;
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		id instance = class_createInstance(objc_getClass("MyClass"), 0);
		object_dispose(instance);
	}
	
	c2 = clock();
	
	return (c2 - c1);
}

int main(int argc, const char *argv[]){
	perform_tests(allocation_test);
	return 0;
}


#include "testing-cocoa.h"


clock_t dispatch_test(void){
	clock_t c1, c2;
	id instance = class_createInstance(objc_getClass("MySubclass"), 0);
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		OBJC_MSG_SEND_DEBUG(instance, increment);
	}
	
	c2 = clock();
	
	object_dispose(instance);
	
	return (c2 - c1);
}

int main(int argc, const char *argv[]){
	perform_tests(dispatch_test);
	return 0;
}


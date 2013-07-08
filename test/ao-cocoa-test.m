
#include "testing-cocoa.h"

clock_t ao_test(void){
	clock_t c1, c2;
	id instance = class_createInstance(objc_getClass("MyClass"), 0);
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		OBJC_MSG_SEND_DEBUG(instance, incrementViaAO);
	}
	
	c2 = clock();
	
	object_dispose(instance);
	
	return (c2 - c1);
}

int main(int argc, const char *argv[]){
	perform_tests(ao_test);
	return 0;
}

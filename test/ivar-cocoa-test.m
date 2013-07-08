
#include "testing-cocoa.h"

clock_t ivar_test(void){
	clock_t c1, c2;
	id instance = class_createInstance(objc_getClass("MySubclass"), 0);
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		OBJC_MSG_SEND_DEBUG(instance, incrementViaSettersAndGetters);
	}
	
	c2 = clock();
	object_dispose(instance);
	
	return (c2 - c1);
}


int main(int argc, const char *argv[]){
	perform_tests(ivar_test);
	return 0;
}


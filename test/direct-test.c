#include "testing.h"
#include <stdio.h>
#include <time.h>

static int i;

void direct_call(void) __attribute__((noinline));
void direct_call(void){
	++i;
}

void(*direct_call_method)(void);

clock_t direct_call_test(void){
	clock_t c1, c2;
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		direct_call_method();
	}
	
	c2 = clock();
	
	return (c2 - c1);
}

int main(int argc, const char * argv[]){
	/** 
	 * Need to make sure that it doesn't get inlined,
	 * and a method is indeed called.
	 */
	direct_call_method = direct_call;
	perform_tests(direct_call_test);
	return 0;
}

#include "selector.h"
#include "utils.h"

int main(int argc, const char * argv[])
{
	for (int i = 1; i < 30000; ++i){
		char *str;
		asprintf(&str, "%i", i);
		
		SEL sel = objc_selector_register(str, "@@:");
		
		objc_assert(objc_strings_equal(objc_selector_get_name(sel), str), "Couldn't get selector back!");
		objc_assert(objc_strings_equal(objc_selector_get_types(sel), "@@:"), "Couldn't get selector's types back!");
		
		free(str);
	}
	
	return 0;
}

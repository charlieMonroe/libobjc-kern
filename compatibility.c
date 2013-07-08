#include "compatibility.h"
#include "selector.h"

SEL sel_registerName(const char *str){
	/* Simply pass the name to the Modular Run-time function. */
	return objc_selector_register(str);
}


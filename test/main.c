#include <stdio.h>
#include <time.h>

#include "runtime.h"
#include "class.h"
#include "method.h"
#include "selector.h"
#include "../classes/MRObjects.h"

#include "../extras/ao-ext.h"
#include "../extras/categs.h"


#define OBJC_HAS_AO_EXTENSION 0
#define OBJC_HAS_CATEGORIES_EXTENSION 1

#define OBJC_INLINE_CACHING 0 /* Complete */

#include "testing.h"

GENERATE_TEST(category, "MySubclass", {}, DISPATCH_ITERATIONS, {
	SEL selector = NULL;
	IMP impl = NULL;
	OBJC_GET_IMP((id)instance, "incrementViaCategoryMethod", selector, impl);
	impl((id)instance, selector);
}, (*((int*)(objc_object_get_variable((id)instance, objc_class_get_ivar(objc_class_for_name("MySubclass"), "i")))) == DISPATCH_ITERATIONS))

int main(int argc, const char * argv[]){
	register_classes();
	list_classes();
	
	category_test();
	
	return 0;
}


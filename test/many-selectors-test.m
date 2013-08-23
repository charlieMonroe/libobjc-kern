
#include "../KKObjects.h"

#include "../kernobjc/runtime.h"
#include "../private.h"
#include "../os.h"
#include "../types.h"
#include "../utils.h"

static BOOL methodCalled = NO;

static char selBuffer[] = "XXXXXXXselectorXXXXXXXX";

static id x(id self, SEL _cmd)
{
	methodCalled = YES;
	objc_assert(strcmp(selBuffer, sel_getName(_cmd)) == 0,
				"Comparison failed.\n");
	return self;
}

void many_selectors_test(void);
void many_selectors_test(void){
	SEL nextSel;
	Class cls = [KKObject class];
	objc_assert(cls != Nil, "class is Nil!\n");
	int sel_size = 0;
	
	/* Should really use 0xf000, but it takes too much time and the kernel
	 * panics in between thinking that there's a deadlock. */
	
	for (uint32_t i=0 ; i<0x1000 ; i++)
	{
		objc_format_string(selBuffer, 16, "%dselector%d", i, i);
		nextSel = sel_registerName(selBuffer, "@@:");
		
		x(nil, nextSel);
		
		sel_size += objc_strlen(selBuffer);
	}
	objc_assert(class_addMethod(object_getClass([KKObject class]), nextSel, (IMP)x),
		   "Couldn't add method!");
	objc_assert(cls == [KKObject class], "Wrong class!\n");
	
	// Test both the C and assembly code paths.
	IMP impl = class_getMethodImplementation(object_getClass((id)cls), nextSel);
	objc_assert(impl != NULL, "NULL impl!\n");
	impl(cls, nextSel);
	objc_assert(methodCalled == YES, "Method not called!\n");
	
	methodCalled = NO;
	objc_msgSend([KKObject class], nextSel);
	objc_assert(methodCalled == YES, "Method not called!\n");
	
	
	objc_log("===================\n");
	objc_log("Passed many selectors tests.\n\n");
}




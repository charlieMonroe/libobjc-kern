#import "../kernobjc/runtime.h"
#import "../KKObjects.h"
#import "../private.h"
#import "../utils.h"
#import "../types.h"

#import "../sarray2.h"
#import "../dtable.h"

#ifdef _KERNEL
	#include <sys/stdarg.h>
#else
	#include <stdarg.h>
#endif


#define assert(x) objc_assert(x, "Wrong value, line %d\n", __LINE__)

id objc_msgSend(id, SEL, ...);
void objc_msgSend_stret(id, SEL, ...);

typedef struct { int a,b,c,d,e; } s;
@interface Fake
- (int)izero;
@end

Class TestCls;
@interface MessageTest : KKObject {
	
}
@end
@implementation MessageTest
- foo
{
	assert((id)1 == self);
	assert(strcmp("foo", sel_getName(_cmd)) == 0);
	return (id)0x42;
}
+ foo
{
	assert(TestCls == self);
	assert(strcmp("foo", sel_getName(_cmd)) == 0);
	return (id)0x42;
}
+ (s)sret
{
	assert(TestCls == self);
	assert(strcmp("sret", sel_getName(_cmd)) == 0);
	s st = {1,2,3,4,5};
	return st;
}
- (s)sret
{
	assert((id)3 == self);
	assert(strcmp("sret", sel_getName(_cmd)) == 0);
	s st = {1,2,3,4,5};
	return st;
}
+ (void)printf: (const char*)str, ...
{
	va_list ap;
	char *s;
	
	va_start(ap, str);
	int (*vasprintf_fn)(char **, const char *, va_list);
	vasprintf_fn = vasprintf;
	vasprintf_fn(&s, str, ap);
	va_end(ap);
	
	assert(objc_strings_equal(s, "Format string 42 \n"));
}
+ (void)initialize
{
	[self printf: "Format %s %d %c", "string", 42, '\n'];
	@throw self;
}
+ nothing { return 0; }
@end

void message_send_test(void);
void message_send_test(void) {
	TestCls = objc_getClass("MessageTest");
	
	int exceptionThrown = 0;
	@try {
		objc_msgSend(TestCls, @selector(foo));
	} @catch (id e)
	{
		assert((TestCls == e) && "Exceptions propagate out of +initialize");
		exceptionThrown = 1;
	}
	objc_log("Beyong try block, class dtable %p, uninstalled at %p\n", ((struct objc_class*)TestCls)->dtable, uninstalled_dtable);
	assert(exceptionThrown && "An exception was thrown");
	assert((id)0x42 == objc_msgSend(TestCls, @selector(foo)));
	objc_msgSend(TestCls, @selector(nothing));
	objc_msgSend(TestCls, @selector(missing));
	assert(0 == objc_msgSend(0, @selector(nothing)));
	id a = objc_msgSend(objc_getClass("MessageTest"), @selector(foo));
	assert((id)0x42 == a);
	a = objc_msgSend(TestCls, @selector(foo));
	assert((id)0x42 == a);
	assert(objc_register_small_object_class(objc_getClass("MessageTest"), 1));
	a = objc_msgSend((id)01, @selector(foo));
	assert((id)0x42 == a);
	s ret = ((s(*)(id, SEL))objc_msgSend_stret)(TestCls, @selector(sret));
	assert(ret.a == 1);
	assert(ret.b == 2);
	assert(ret.c == 3);
	assert(ret.d == 4);
	assert(ret.e == 5);
	if (sizeof(id) == 8)
	{
		assert(objc_register_small_object_class(objc_getClass("MessageTest"), 3));
		ret = ((s(*)(id, SEL))objc_msgSend_stret)((id)3, @selector(sret));
		assert(ret.a == 1);
		assert(ret.b == 2);
		assert(ret.c == 3);
		assert(ret.d == 4);
		assert(ret.e == 5);
	}
	Fake *f = nil;
	assert(0 == [f izero]);
}
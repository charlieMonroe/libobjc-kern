#include "../kernobjc/runtime.h"
#include "../types.h"
#include "../KKObjects.h"

@interface Forward : KKObject
- (id)forwardingTargetForSelector: (SEL)sel;
@end

id target;

@implementation Forward
- (id)forwardingTargetForSelector: (SEL)sel
{
	return target;
}
@end

@interface Forward2 : KKObject
@end

@interface ForwardingTarget : KKObject
@end

BOOL forwardingTargetCalled;

@implementation ForwardingTarget
- (void)foo: (int)x
{
	objc_assert(x == 42, "x != 42\n");
	forwardingTargetCalled = YES;
}
@end
@implementation Forward2
- (void)forward: (int)x
{
	[target foo: x];
}
@end

static id proxy_lookup(id receiver, SEL selector)
{
	if (class_respondsToSelector(object_getClass(receiver), @selector(forwardingTargetForSelector:)))
	{
		return [receiver forwardingTargetForSelector: selector];
	}
	return nil;
}

static struct objc_slot* forward(id receiver, SEL selector)
{
	/* Should be __thread, or ideally use objc_get_tls_data. */
	static struct objc_slot forwardingSlot;
	if (class_respondsToSelector(object_getClass(receiver), @selector(forward:)))
	{
		forwardingSlot.implementation = class_getMethodImplementation(object_getClass(receiver), @selector(forward:));
		return &forwardingSlot;
	}
	objc_assert(0, "Not reachable.\n");
}

void forwarding_test(void);
void forwarding_test(void){
	objc_proxy_lookup = proxy_lookup;
	__objc_msg_forward3 = forward;
	
	target = [ForwardingTarget new];
	id proxy = [Forward new];
	[proxy foo: 42];
	[proxy dealloc];
	objc_assert(forwardingTargetCalled == YES, "Forwarding target not called\n");
	forwardingTargetCalled = NO;
	proxy = [Forward2 new];
	[proxy foo: 42];
	[proxy dealloc];
	objc_assert(forwardingTargetCalled == YES, "Forwarding target not called\n");
	
	objc_log("===================\n");
	objc_log("Passed forwarding tests.\n\n");
	
	objc_proxy_lookup = NULL;
	__objc_msg_forward3 = NULL;
}


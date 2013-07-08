
#ifndef _TEST_COCOA_H_
#define _TEST_COCOA_H_

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdio.h>
#include <time.h>

#if OBJC_FORWARDING_TEST && !NONFOUNDATION_FORWARDING
#import <Foundation/Foundation.h>
@interface MyClass : NSObject {
@public
#else
@interface MyClass {
@public
	id isa;
#endif
	id proxyObject;
	int i;
}

-(void)increment;
-(void)incrementViaAO;
-(void)incrementViaSettersAndGetters;

@end

@implementation MyClass

-(void)increment{
	++self->i;
}
-(void)incrementViaAO{
	unsigned int old_value = (unsigned int)objc_getAssociatedObject(self, (void*)_cmd);
	objc_setAssociatedObject(self, (void*)_cmd, (id)(old_value + 1), OBJC_ASSOCIATION_ASSIGN);
}
-(void)incrementViaSettersAndGetters{
	Ivar ivar = class_getInstanceVariable(self->isa, "i");
	unsigned int old_value = (unsigned int)object_getIvar(self, ivar);
	object_setIvar(self, ivar, (id)(old_value + 1));
}
	
	
#if NONFOUNDATION_FORWARDING

-(id)forward:(SEL)sel :(marg_list)args{
	// We're really just supporting -unknownSelector.
	return objc_msgSend(proxyObject, sel);
}
	
#else
	
/**
 * The Foundation wraps the forwarding into NSInvocation.
 */
	
// Just to silence the warning about an unknown selector
+(id)signatureWithObjCTypes:(const char*)types{
	return nil;
}
-(void)forwardInvocation:(id)operation{
	[operation invokeWithTarget:self->proxyObject];
}

// Just to silence the warning about an unknown selector
-(void)invokeWithTarget:(id)target{
	
}
-(id)methodSignatureForSelector:(SEL)selector{
	// We're only forwarding -unknownSelector:
	return [objc_getClass("NSMethodSignature") signatureWithObjCTypes:"v@:"];
}
#endif
	
@end
	
#if OBJC_HAS_CATEGORIES_EXTENSION

@interface MyClass (Privates)
-(void)incrementViaCategoryMethod;
@end
@implementation MyClass (Privates)
-(void)incrementViaCategoryMethod{
	++self->i;
}
@end
	
#endif


@interface MySubclass : MyClass
-(void)incrementWithSuper;
@end

@implementation MySubclass
-(void)incrementWithSuper{
	++self->i;
	[super increment];
}
@end

	
#if OBJC_FORWARDING_TEST && !NONFOUNDATION_FORWARDING
@interface NewClass : NSObject {
#else
@interface NewClass{
	id isa;
#endif
	
}

-(void)unknownSelector;

@end

@implementation NewClass

-(void)unknownSelector{
	/** No-op */
}
@end

#define OBJC_CACHING_NONE 0
#define OBJC_CACHING_SELECTOR 1
// There's nothing else to cache

#if OBJC_CACHING == OBJC_CACHING_NONE
#define OBJC_MSG_SEND_DEBUG(obj, sel) {\
	objc_msgSend(obj, sel_registerName(#sel));\
}
#elif OBJC_CACHING == OBJC_CACHING_SELECTOR
#define OBJC_MSG_SEND_DEBUG(obj, sel) {\
	[obj sel];\
}
#else
#error Unknown caching.
#endif
	
#include "testing-common.h"

#endif /* _TEST_COCOA_H_ */

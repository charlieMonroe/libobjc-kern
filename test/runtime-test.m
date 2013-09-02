#import "../os.h"
#import "../kernobjc/runtime.h"
#import "../private.h"

static int exitStatus = 0;

static void _test(BOOL X, char *expr, int line)
{
  if (!X)
  {
    exitStatus = 1;
    objc_abort("ERROR: Test failed: '%s' on %s:%d\n", expr, __FILE__, line);
  }
}
#define test(X) _test(X, #X, __LINE__)

static int stringsEqual(const char *a, const char *b)
{
  return 0 == strcmp(a,b);
}

@protocol NSCoding
@end

#ifdef __has_attribute
#if __has_attribute(objc_root_class)
__attribute__((objc_root_class))
#endif
#endif
@interface __NSObject <NSCoding>
{
	id isa;
	int refcount;
}
@end
@implementation __NSObject
- (id)class
{
	return object_getClass(self);
}
+ (id)class
{
	return self;
}
+ (id)new
{
	return class_createInstance(self, 0);
}
- (void)release
{
	if (refcount == 0)
	{
		refcount--;
		object_dispose(self);
	}else{
		refcount--;
	}

}
- (id)retain
{
	refcount++;
	return self;
}
@end


@interface FooRT : __NSObject
{
  id a;
}
- (void) aMethod;
+ (void) aMethod;
- (int) manyTypes;
- (void) synchronizedCode;
+ (void) synchronizedCode;
+ (id) shared;
- (BOOL) basicThrowAndCatchException;
@end

@interface Bar : FooRT
{
  id b;
}
- (void) anotherMethod;
+ (void) anotherMethod;
- (id) manyTypes;
- (id) aBool: (BOOL)d andAnInt: (int) w;
@end

id exceptionObj = @"Exception";

@implementation FooRT
- (void) aMethod
{
}
+ (void) aMethod
{
}
- (int) manyTypes
{
  return YES;
}
- (void) synchronizedCode
{
	@synchronized(self) { [[self class] synchronizedCode]; }
}
+ (void) synchronizedCode
{
	@synchronized(self) { }
}
+ (id) shared
{
	@synchronized(self) { }
	return nil;
}
- (void) throwException
{
	@throw exceptionObj;
}
- (BOOL) basicThrowAndCatchException
{
	@try
	{
		[self throwException];
	}
	@catch (id e)
	{
		test(e == exceptionObj);
	}
	@finally
	{
		return YES;
	}
	return NO;
}
@end

@implementation Bar
- (void) anotherMethod
{
}
+ (void) anotherMethod
{
}
- (id) manyTypes
{
  return @"Hello";
}
- (id) aBool: (BOOL)d andAnInt: (int) w
{
  return @"Hello";
}
@end


static void testInvalidArguments()
{
  test(NO == class_conformsToProtocol([__NSObject class], NULL));
  test(NO == class_conformsToProtocol(Nil, NULL));
  test(NO == class_conformsToProtocol(Nil, @protocol(NSCoding)));
  test(NULL == class_copyIvarList(Nil, NULL));
  test(NULL == class_copyMethodList(Nil, NULL));
  test(NULL == class_copyPropertyList(Nil, NULL));
  test(NULL == class_copyProtocolList(Nil, NULL));
  test(nil == class_createInstance(Nil, 0));
  test(0 == class_getVersion(Nil));
  test(NO == class_isMetaClass(Nil));
  test(Nil == class_getSuperclass(Nil));

  test(null_selector == method_getName(NULL));
  test(NULL == method_copyArgumentType(NULL, 0));
  test(NULL == method_copyReturnType(NULL));
  method_exchangeImplementations(NULL, NULL);
  test((IMP)NULL == method_setImplementation(NULL, (IMP)NULL));
  test((IMP)NULL == method_getImplementation(NULL));
  method_getArgumentType(NULL, 0, NULL, 0);
  test(0 == method_getNumberOfArguments(NULL));
  test(NULL == method_getTypeEncoding(NULL));
  method_getReturnType(NULL, NULL, 0);

  test(NULL == ivar_getName(NULL));
  test(0 == ivar_getOffset(NULL));
  test(NULL == ivar_getTypeEncoding(NULL));

  test(nil == objc_getProtocol(NULL));

  test(stringsEqual("<null selector>", sel_getName((SEL)0)));
  test(YES == sel_isEqual((SEL)0, (SEL)0));

  test(NULL == property_getName(NULL));

  objc_log("testInvalidArguments() ran\n");
}

static void testAMethod(Method m)
{
  test(NULL != m);
  test(stringsEqual("aMethod", sel_getName(method_getName(m))));

  objc_log("testAMethod() ran\n");
}

static void testGetMethod()
{
  testAMethod(class_getClassMethod([Bar class], @selector(aMethod)));
  testAMethod(class_getClassMethod([Bar class], sel_getNamed("aMethod")));

  objc_log("testGetMethod() ran\n");
}

static void testProtocols()
{
  test(protocol_isEqual(@protocol(NSCoding), objc_getProtocol("NSCoding")));

  objc_log("testProtocols() ran\n");
}

static void testClassHierarchy()
{
  Class nsProxy = objc_getClass("NSProxy");
  Class nsObject = objc_getClass("__NSObject");
  Class nsProxyMeta = object_getClass(nsProxy);
  Class nsObjectMeta = object_getClass(nsObject);

  test(object_getClass(nsProxyMeta) == nsProxyMeta);
  test(object_getClass(nsObjectMeta) == nsObjectMeta);

  test(Nil == class_getSuperclass(nsProxy));
  test(Nil == class_getSuperclass(nsObject));

  test(nsObject == class_getSuperclass(nsObjectMeta));
  test(nsProxy == class_getSuperclass(nsProxyMeta));
  objc_log("testClassHierarchy() ran\n");
}

static void testAllocateClass()
{
  Class newClass = objc_allocateClassPair(objc_lookUpClass("__NSObject"), "UserAllocated", 0);
  test(Nil != newClass);
  // class_getSuperclass() will call objc_resolve_class().
  // Although we have not called objc_registerClassPair() yet, this works with
  // the Apple runtime and GNUstep Base relies on this behavior in
  // GSObjCMakeClass().
  test(objc_lookUpClass("__NSObject") == class_getSuperclass(newClass));
  objc_log("testAllocateClass() ran\n");
}

static void testSynchronized()
{
  FooRT *foo = [FooRT new];
  objc_log("Enter synchronized code\n");
  [foo synchronizedCode];
  [foo release];
  [FooRT shared];
  objc_log("testSynchronized() ran\n");
}

static void testExceptions()
{
  FooRT *foo = [FooRT new];
  test([foo basicThrowAndCatchException]);
  [foo release];
  objc_log("testExceptions() ran\n");

}

@interface SlowInit1 : __NSObject
+ (void)doNothing;
@end
@interface SlowInit2 : __NSObject
+ (void)doNothing;
@end

@implementation SlowInit1
+ (void)initialize
{
	objc_sleep(1);
	[SlowInit2 doNothing];
}
+ (void)doNothing {}
@end
static int initCount;
@implementation SlowInit2
+ (void)initialize
{
	objc_sleep(1);
	__sync_fetch_and_add(&initCount, 1);
}
+ (void)doNothing {}
@end



void runtime_test(void);
void runtime_test(void)
{
	testInvalidArguments();
	testGetMethod();
	testProtocols();
	testClassHierarchy();
	testAllocateClass();
	objc_log("Instance of __NSObject: %p\n", class_createInstance([__NSObject class], 0));
	
	testSynchronized();
	testExceptions();
	
	objc_assert(exitStatus == 0, "Exit status != 0\n");
	
	
	objc_log("===================\n");
	objc_log("Passed runtime test.\n\n");
	
}

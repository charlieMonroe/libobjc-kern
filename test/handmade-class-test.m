#include "../kernobjc/runtime.h"
#import "../KKObjects.h"
#include "../selector.h"
#include "../os.h"

static const char *my_class_name = "MyClass";
static const char *my_ivar_name = "my_ivar";

typedef struct{
	id isa;
	int retain_count;
	id my_ivar;
} MyClass;

static BOOL gotInitialized = NO;

static void initialize(Class self, SEL _cmd){
	objc_assert(self == (Class)objc_getClass(my_class_name), "Sending initialize to the wrong class");
	gotInitialized = YES;
}

static id getIvar(MyClass *self, SEL _cmd){
	return self->my_ivar;
}

static void setIvar(MyClass *self, SEL _cmd, id value){
	self->my_ivar = value;
}

void handmade_class_test(void);
void handmade_class_test(void){
  typedef struct {
    Class isa;
    int retain_count;
  } Object;
  
	Class cl = objc_allocateClassPair([KKObject class], my_class_name, 0);
	Class meta_cl = object_getClass((id)cl);
	
	SEL getIvarSel = sel_registerName("getIvar", "@10@0:8");
	SEL setIvarSel = sel_registerName("setIvar", "v24@0:8@16");
	
	class_addIvar(cl, my_ivar_name, sizeof(id), __alignof(id), "@");
	class_addMethod(cl, getIvarSel, (IMP)getIvar, sel_getTypes(getIvarSel));
	class_addMethod(cl, setIvarSel, (IMP)setIvar, sel_getTypes(setIvarSel));
	class_addMethod(meta_cl, objc_initialize_selector, (IMP)initialize,
					sel_getTypes(objc_initialize_selector));
	
	objc_registerClassPair(cl);
	
	
  id instance = [[cl alloc] init];
	
	objc_assert(gotInitialized, "The class did not get initialized!\n");
	
	objc_assert(((Object*)instance)->isa == cl, "Instance is of the wrong class (%s)\n", object_getClassName(instance));
	
	id var = objc_msgSend(instance, getIvarSel);
	objc_assert(var == nil, "var != nil (%p)\n", var);
	
	id targetValue = (id)0x123456;
	var = objc_msgSend(instance, setIvarSel, targetValue);
	objc_assert(((MyClass*)instance)->my_ivar == targetValue, "instance->my_ivar != targetValue (%p)\n", ((MyClass*)instance)->my_ivar);
	
	var = objc_msgSend(instance, getIvarSel);
	objc_assert(var == targetValue, "var != targetValue (%p)\n", var);
	
	
	objc_log("===================\n");
	objc_log("Passed handmade class tests.\n\n");
}

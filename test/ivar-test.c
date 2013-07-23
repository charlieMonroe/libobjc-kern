#include "../classes/MRObjects.h"
#include "../kernobjc/runtime.h"
#include "../os.h"

void ivar_test(void){
	MRObject_instance_t *obj = (MRObject_instance_t*)objc_msgSend((id)&MRObject_class, sel_registerName("alloc", "@@:"));
	obj = (MRObject_instance_t*)objc_msgSend((id)obj, sel_registerName("init", "@@:"));
	
	Ivar isa_ivar = class_getInstanceVariable(&MRObject_class, "isa");
	id isa_ivar_value = object_getIvar((id)obj, isa_ivar);
	
	objc_assert((void*)isa_ivar_value == (void*)&MRObject_class, "Getting wrong isa value!\n");
	
	Ivar retain_count_ivar = class_getInstanceVariable(&MRObject_class, "retain_count");
	id retain_count_value = object_getIvar((id)obj, retain_count_ivar);
	
	objc_assert(retain_count_value == (id)0, "Getting wrong retain count value!\n");
	
	id target_isa_value = (id)0x123456;
	
	object_setIvar((id)obj, isa_ivar, target_isa_value);
	
	isa_ivar_value = object_getIvar((id)obj, isa_ivar);
	
	objc_assert(isa_ivar_value == target_isa_value, "Getting wrong isa value!\n");
	objc_assert((id)obj->isa == target_isa_value, "Getting wrong isa value!\n");
	
	objc_log("===================\n");
	objc_log("Passed ivar tests.\n\n");

}

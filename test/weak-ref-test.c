#include "../class_registry.h"
#include "../class.h"
#include "../classes/MRObjects.h"
#include "../message.h"
#include "../arc.h"
#include "associative.h"

void weak_ref_test(void){
	MRObject_instance_t *obj = (MRObject_instance_t*)objc_msgSend((id)&MRObject_class, sel_registerName("alloc", "@@:"));
	obj = (MRObject_instance_t*)objc_msgSend((id)obj, sel_registerName("init", "@@:"));
	
	id weak_ref = (id)0x1234;
	
	objc_store_weak(&weak_ref, (id)obj);
	
	objc_assert(weak_ref == (id)obj, "The obj wasn't stored in the weak ref!\n");
	
	objc_release((id)obj);
	
	objc_assert(obj->retainCount == -1, "The associated object should have been deallocated!\n");
	objc_assert(weak_ref == 0, "The weak ref isn't zeroed out!\n")
	
	objc_log("===================\n");
	objc_log("Passed weak ref tests.\n\n");

}

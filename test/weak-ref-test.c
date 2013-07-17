#include "../class_registry.h"
#include "../class.h"
#include "../classes/MRObjects.h"
#include "../message.h"
#include "../arc.h"
#include "../types.h"
#include "associative.h"

int main(int argc, const char * argv[]){
	MRObject_instance_t *obj = (MRObject_instance_t*)objc_msg_send((id)&MRObject_class, objc_selector_register("alloc", "@@:"));
	obj = (MRObject_instance_t*)objc_msg_send((id)obj, objc_selector_register("init", "@@:"));
	
	id weak_ref = (id)0x1234;
	
	objc_store_weak(&weak_ref, (id)obj);
	
	objc_assert(weak_ref == (id)obj, "The obj wasn't stored in the weak ref!\n");
	
	// TODO this should be called automatically
	objc_remove_associated_objects((id)obj);
	
	objc_release((id)obj);
	
	objc_assert(obj->retainCount == -1, "The associated object should have been deallocated!\n");
	objc_assert(weak_ref == 0, "The weak ref isn't zeroed out!\n")
	
	objc_log("===================\n");
	objc_log("Passed weak ref tests.\n");
	
	return 0;
}

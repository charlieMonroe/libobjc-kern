#include "../class_registry.h"
#include "../class.h"
#include "../classes/MRObjects.h"
#include "../message.h"
#include "../associative.h"
#include "../types.h"

void associated_objects_test(void){
	MRObject_instance_t *obj = (MRObject_instance_t*)objc_msg_send((id)&MRObject_class, objc_selector_register("alloc", "@@:"));
	obj = (MRObject_instance_t*)objc_msg_send((id)obj, objc_selector_register("init", "@@:"));
	
	void *key = (void*)0x123456;
	
	MRObject_instance_t *obj_to_associate = (MRObject_instance_t*)objc_msg_send((id)&MRObject_class, objc_selector_register("alloc", "@@:"));
	obj_to_associate = (MRObject_instance_t*)objc_msg_send((id)obj, objc_selector_register("init", "@@:"));
	
	objc_set_associated_object((id)obj, key, (id)obj_to_associate, OBJC_ASSOCIATION_ASSIGN);
	objc_set_associated_object((id)obj, key, nil, OBJC_ASSOCIATION_ASSIGN);
	
	objc_assert(obj->isa->flags.fake, "The object should now be of a fake class!\n");
	objc_assert(!objc_object_get_class((id)obj)->flags.fake, "objc_object_get_class() returned a fake class!\n");
	objc_assert(obj_to_associate->retainCount == 0, "The associated object shouldn't have been released!\n");
	
	objc_set_associated_object((id)obj, key, (id)obj_to_associate, OBJC_ASSOCIATION_RETAIN);
	objc_set_associated_object((id)obj, key, nil, OBJC_ASSOCIATION_ASSIGN);
	
	objc_assert(obj_to_associate->retainCount == 0, "The associated object should have been released!\n");
	
	objc_log("===================\n");
	objc_log("Passed associated object tests.\n\n");
}

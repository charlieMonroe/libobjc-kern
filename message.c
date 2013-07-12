#include "message.h"
#include "dtable.h"
#include "class.h"

Slot objc_class_get_slot(Class cl, SEL selector){
	Slot slot = objc_dtable_lookup(cl->dtable, selector);
	if (slot == NULL){
		dtable_t dtable = dtable_for_class(cl);
		if (dtable == uninstalled_dtable){
			dtable = dtable_for_class(cl);
			
		}
		slot = objc_dtable_lookup(dtable, selector);
		
		// Nothing else for us to do.
	}
	return slot;
}

id objc_msg_send(id receiver, SEL selector, ...){
	
	// TODO passing arguments
	
	if (receiver == nil){
		objc_debug_log("objc_msg_send - nil receiver.\n");
		return nil;
	}
	
	if (receiver->isa->dtable == NULL){
		// TODO install the initial dtable on classes
		receiver->isa->dtable = uninstalled_dtable;
	}
	
	Slot sl = objc_class_get_slot(receiver->isa, selector);
	if (sl == NULL || sl->method == NULL){
		objc_debug_log("Installing dtable on class %s%s.\n", receiver->isa->name, receiver->isa->flags.meta ? " (meta)" : "");
		
		Class cl = receiver->isa;
		dtable_t dtable = dtable_for_class(cl);
		/* Install the dtable if it hasn't already been initialized. */
		if (dtable == uninstalled_dtable){
			// TODO
			objc_send_initialize(*receiver);
			dtable = dtable_for_class(cl);
			sl = objc_dtable_lookup(dtable, selector);
		}else{
			// Check again incase another thread updated the dtable while we
			// weren't looking
			sl = objc_dtable_lookup(dtable, selector);
		}
		if (NULL == sl){
			// TODO forwarding
			objc_assert(NO, "TODO forwarding");
		}
		
		
		
		/*// Find the implementation and fixup the cache
		// TODO more elegant solution
		Method m = objc_object_lookup_method(receiver, selector);
		return m->implementation(receiver, selector);
		
		
		add_method_list_to_class(receiver->isa, receiver->isa->methods);
	
		// Get the slot again
		sl = objc_class_get_slot(receiver->isa, selector);*/
		objc_assert(sl != NULL, "Eh?");
	}
	return sl->method(receiver, selector);
}


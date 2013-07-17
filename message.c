#include "message.h"
#include "dtable.h"
#include "class.h"

PRIVATE Slot objc_class_get_slot(Class cl, SEL selector){
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
	
	Slot sl = objc_class_get_slot(receiver->isa, selector);
	if (sl == NULL || sl->method == NULL){
		objc_install_dtable_for_object(receiver);
		
		// Now that the dtable has been installed, try it again
		Class cl = receiver->isa;
		while (cl != Nil){
			sl = objc_class_get_slot(cl, selector);
			if (sl != NULL){
				break;
			}
			cl = cl->super_class;
		}
		
		if (sl == NULL){
			// TODO forwarding
			objc_assert(NO, "TODO forwarding");
		}
		
		objc_assert(sl != NULL, "Shouldn't be here - the forwarding should have taken care of this case!");
	}
	return sl->method(receiver, selector);
}


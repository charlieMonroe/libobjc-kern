#include "message.h"
#include "dtable.h"

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


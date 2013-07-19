#include "message.h"
#include "dtable.h"
#include "class.h"

static long long nil_method(id self, SEL _cmd) { return 0; }
static long double nil_method_D(id self, SEL _cmd) { return 0; }
static double nil_method_d(id self, SEL _cmd) { return 0; }
static float nil_method_f(id self, SEL _cmd) { return 0; }

static struct objc_slot nil_slot = { Nil, Nil, (IMP)nil_method, 0, 1, 0 };
static struct objc_slot nil_slot_D = { Nil, Nil, (IMP)nil_method_D, 0, 1, 0 };
static struct objc_slot nil_slot_d = { Nil, Nil, (IMP)nil_method_d, 0, 1, 0 };
static struct objc_slot nil_slot_f = { Nil, Nil, (IMP)nil_method_f, 0, 1, 0 };


// Default implementations of the two new hooks.  Return NULL.
static id objc_proxy_lookup_null(id receiver, SEL op) { return nil; }
static Slot objc_msg_forward3_null(id receiver, SEL op) { return &nil_slot; }

id (*objc_proxy_lookup)(id receiver, SEL op) = objc_proxy_lookup_null;
Slot (*__objc_msg_forward3)(id receiver, SEL op) = objc_msg_forward3_null;

Slot objc_msg_lookup_sender(id *receiver, SEL selector, id sender);

static
// Uncomment for debugging
//__attribute__((noinline))
__attribute__((always_inline))
Slot objc_msg_lookup_internal(id *receiver,
                                SEL selector,
                                id sender)
{
	Class class = objc_object_get_class_inline((*receiver));
	Slot result = objc_dtable_lookup(class->dtable, selector);
	//TODO UNLIKELY
	if ((NULL == result))
	{
		dtable_t dtable = dtable_for_class(class);
		/* Install the dtable if it hasn't already been initialized. */
		if (dtable == uninstalled_dtable)
		{
			objc_send_initialize(*receiver);
			dtable = dtable_for_class(class);
			result = objc_dtable_lookup(dtable, selector);
		}
		else
		{
			// Check again incase another thread updated the dtable while we
			// weren't looking
			result = objc_dtable_lookup(dtable, selector);
		}
		if (0 == result)
		{
			id newReceiver = objc_proxy_lookup(*receiver, selector);
			// If some other library wants us to play forwarding games, try
			// again with the new object.
			if (nil != newReceiver)
			{
				*receiver = newReceiver;
				return objc_msg_lookup_sender(receiver, selector, sender);
			}
			if (0 == result)
			{
				result = __objc_msg_forward3(*receiver, selector);
			}
		}
	}
	return result;
}


PRIVATE
IMP slowMsgLookup(id *receiver, SEL cmd)
{
	return objc_msg_lookup_sender(receiver, cmd, nil)->implementation;
}

Slot (*objc_plane_lookup)(id *receiver, SEL op, id sender) = objc_msg_lookup_internal;

Slot objc_msg_lookup_sender_non_nil(id *receiver, SEL selector, id sender){
	return objc_msg_lookup_internal(receiver, selector, sender);
}

/**
 * New Objective-C lookup function.  This permits the lookup to modify the
 * receiver and also supports multi-dimensional dispatch based on the sender.
 */
Slot objc_msg_lookup_sender(id *receiver, SEL selector, id sender){
	// Returning a nil slot allows the caller to cache the lookup for nil too,
	// although this is not particularly useful because the nil method can be
	// inlined trivially.
	// TODO unlikely
	if (*receiver == nil){
		// Return the correct kind of zero, depending on the type encoding.
		const char *types = objc_selector_get_types(selector);
		const char *t = types;
		// Skip type qualifiers
		while ('r' == *t || 'n' == *t || 'N' == *t || 'o' == *t ||
		       'O' == *t || 'R' == *t || 'V' == *t){
			++t;
		}
		
		switch (types[0]){
			case 'D': return &nil_slot_D;
			case 'd': return &nil_slot_d;
			case 'f': return &nil_slot_f;
		}
		return &nil_slot;
	}
	
	return objc_msg_lookup_internal(receiver, selector, sender);
}

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

Slot objc_slot_lookup_super(struct objc_super *super, SEL selector){
	if (super->receiver == nil){
		return &nil_slot;
	}
	
	Slot sl = objc_dtable_lookup(dtable_for_class(super->class), selector);
	if (sl == NULL){
		Class cl = objc_object_get_class_inline(super->receiver);
		if (dtable_for_class(cl) == uninstalled_dtable){
			if (cl->flags.meta){
				objc_send_initialize(super->receiver);
			}else{
				objc_send_initialize((id)super->receiver->isa);
			}
			return objc_slot_lookup_super(super, selector);
		}
		sl = &nil_slot;
	}
	return sl;
}


id objc_msg_send(id receiver, SEL selector, ...){
	
	// TODO passing arguments
	
	if (receiver == nil){
		objc_debug_log("objc_msg_send - nil receiver.\n");
		return nil;
	}
	
	Class cl = objc_object_get_class_inline(receiver);
	Slot sl = objc_class_get_slot(cl, selector);
	if (sl == NULL || sl->implementation == NULL){
		objc_install_dtable_for_object(receiver);
		sl = objc_class_get_slot(cl, selector);
		
		if (sl == NULL){
			// TODO forwarding
			objc_assert(NO, "TODO forwarding");
		}
		
		objc_assert(sl != NULL, "Shouldn't be here - the forwarding should have taken care of this case!");
	}
	return sl->implementation(receiver, selector);
}


#pragma mark -
#pragma mark Responding to selectors

BOOL class_respondsToSelector(Class cl, SEL selector){
	if (cl == Nil || selector == null_selector){
		return NO;
	}
	return objc_class_get_slot(cl, selector) != NULL;
}

#pragma mark -
#pragma mark Regular lookup functions

IMP class_getMethodImplementation(Class cl, SEL selector){
	/** No forwarding here! This is simply to lookup
	 * a method implementation.
	 */
	return objc_class_get_slot(cl, selector)->implementation;
}

IMP class_getMethodImplementation_stret(Class cls, SEL name){
	return class_getMethodImplementation(cls, name);
}


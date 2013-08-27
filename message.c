#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "kernobjc/message.h"
#include "selector.h"
#include "message.h"
#include "kernobjc/runtime.h"
#include "sarray2.h"
#include "dtable.h"
#include "class.h"
#include "private.h"

/*
 * Static functions and slots for nil receivers.
 */
static long long nil_method(id self, SEL _cmd) { return 0; }
//static long double nil_method_D(id self, SEL _cmd) { return 0; }
//static double nil_method_d(id self, SEL _cmd) { return 0; }
//static float nil_method_f(id self, SEL _cmd) { return 0; }

static struct objc_slot nil_slot = { Nil, Nil, (IMP)nil_method, 0, 1, 0 };
//static struct objc_slot nil_slot_D = { Nil, Nil, (IMP)nil_method_D, 0, 1, 0 };
//static struct objc_slot nil_slot_d = { Nil, Nil, (IMP)nil_method_d, 0, 1, 0 };
//static struct objc_slot nil_slot_f = { Nil, Nil, (IMP)nil_method_f, 0, 1, 0 };


/* Default proxy lookup. */
static id
objc_proxy_lookup_null(id receiver, SEL op)
{
	return nil;
}

/* Default forwarding hook. */
static struct objc_slot *
objc_msg_forward3_null(id receiver, SEL op)
{
	return &nil_slot;
}

/* Forward declarations. */
struct objc_slot *objc_msg_lookup_sender(id *receiver, SEL selector, id sender);
static struct objc_slot *objc_msg_lookup_internal(id *receiver, SEL selector, id sender);

/*
 * The proxy lookup. When the method isn't cached the slow msg lookup goes on
 * to ask the proxy hook to supply a different receiver. If none is supplied,
 * the dispatch goes to the the __objc_msg_forward3 hook.
 */
id (*objc_proxy_lookup)(id receiver, SEL op) = objc_proxy_lookup_null;

/*
 * When slow method lookup fails and the proxy lookup hook returns no receiver,
 * the dispatch tries this hook for forwarding.
 */
struct objc_slot *(*__objc_msg_forward3)(id receiver, SEL op)
= objc_msg_forward3_null;

/*
 * An export of the other otherwise
 */
struct objc_slot *(*objc_plane_lookup)(id *receiver, SEL op, id sender) =
objc_msg_lookup_internal;




static
/* Uncomment for debugging */
/* __attribute__((noinline)) */
__attribute__((always_inline)) struct objc_slot *
objc_msg_lookup_internal(id *receiver, SEL selector, id sender)
{
	Class class = objc_object_get_class_inline((*receiver));
	struct objc_slot *result = objc_dtable_lookup(class->dtable, selector);
	if (UNLIKELY(NULL == result)) {
		dtable_t dtable = dtable_for_class(class);
		/* Install the dtable if it hasn't already been initialized. */
		if (dtable == uninstalled_dtable){
			objc_debug_log("Sending initialize to receiver %p, selector %s[%d]\n", *receiver, sel_getName(selector), (unsigned)selector);
			objc_send_initialize(*receiver);
			dtable = dtable_for_class(class);
			result = objc_dtable_lookup(dtable, selector);
		}else{
			/*
			 * Check again in case another thread updated the dtable
			 * while we weren't looking.
			 */
			result = objc_dtable_lookup(dtable, selector);
		}
		if (NULL == result) {
			id newReceiver = objc_proxy_lookup(*receiver, selector);
			/*
			 * If some other library wants us to play forwarding
			 * games, try again with the new object.
			 */
			if (nil != newReceiver) {
				*receiver = newReceiver;
				return objc_msg_lookup_sender(receiver,
											  selector,
											  sender);
			} else {
				result = __objc_msg_forward3(*receiver,
											 selector);
			}
		}
	}
	return result;
}


PRIVATE IMP
slowMsgLookup(id *receiver, SEL cmd)
{
	objc_debug_log("slowMsgLookup: selector (%i), receiver (%p)\n", (int)cmd,
				   *receiver);
	return objc_msg_lookup_sender(receiver, cmd, nil)->implementation;
}



struct objc_slot *
objc_msg_lookup_sender_non_nil(id *receiver, SEL selector, id sender)
{
	return objc_msg_lookup_internal(receiver, selector, sender);
}

/*
 * New Objective-C lookup function.  This permits the lookup to modify the
 * receiver and also supports multi-dimensional dispatch based on the sender.
 */
struct objc_slot *
objc_msg_lookup_sender(id *receiver, SEL selector, id sender)
{
	/*
	 * Returning a nil slot allows the caller to cache the lookup for nil
	 * too, although this is not particularly useful because the nil method
	 * can be inlined trivially.
	 */
	if (UNLIKELY(*receiver == nil)){
		/*
		 * Return the correct kind of zero, depending on the type
		 * encoding.
		 */
		const char *types = sel_getTypes(selector);
		const char *t = types;
		/* Skip type qualifiers */
		while ('r' == *t || 'n' == *t || 'N' == *t || 'o' == *t ||
		       'O' == *t || 'R' == *t || 'V' == *t){
			++t;
		}
		
		switch (types[0]){
			case 'D':
			case 'd':
			case 'f':
				objc_abort("Float or double type in the kernel.\n");
				break;
		}
		return &nil_slot;
	}
	
	return objc_msg_lookup_internal(receiver, selector, sender);
}

PRIVATE struct objc_slot *
objc_get_slot(Class cl, SEL selector)
{
	objc_debug_log("Getting slot on class %s [meta=%s; fake=%s]\n",
				   class_getName(cl),
				   cl->flags.meta ? "YES" : "NO",
				   cl->flags.fake ? "YES" : "NO");
	struct objc_slot *slot = objc_dtable_lookup(cl->dtable, selector);
	if (slot == NULL){
		dtable_t dtable = dtable_for_class(cl);
		if (dtable == uninstalled_dtable){
			dtable = dtable_for_class(cl);
		}
		
		slot = objc_dtable_lookup(dtable, selector);
	}
	return slot;
}

PRIVATE struct objc_slot *
objc_slot_lookup_super(struct objc_super *super, SEL selector)
{
	if (super->receiver == nil){
		return &nil_slot;
	}
	
	struct objc_slot *sl = objc_dtable_lookup(dtable_for_class(super->class), selector);
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

static inline IMP
_class_getIMPForSelector(Class cl, SEL selector){
	if (cl == Nil || selector == null_selector){
		return NULL;
	}
	if (cl->dtable != uninstalled_dtable){
		struct objc_slot *slot = objc_get_slot(cl, selector);
		return slot == NULL ? NULL : slot->implementation;
	}else{
		/* Need to find it manually */
		if (cl->flags.fake) {
			/* Fake classes only have dtables. */
			cl = objc_class_get_nonfake_inline(cl);
		}
		
		struct objc_method_list_struct *method_list = cl->methods;
		while (method_list != NULL){
			for (int i = 0; i < method_list->size; ++i){
				if (method_list->list[i].selector == selector){
					return method_list->list[i].implementation;
				}
			}
			method_list = method_list->next;
		}
		
		return _class_getIMPForSelector(cl, selector);
	}
}


#pragma mark -
#pragma mark Responding to selectors

BOOL
class_respondsToSelector(Class cl, SEL selector)
{
	return _class_getIMPForSelector(cl, selector) != NULL;
}

#pragma mark -
#pragma mark Regular lookup functions

IMP
class_getMethodImplementation(Class cl, SEL selector)
{
	/*
	 * No forwarding here! This is simply to lookup
	 * a method implementation.
	 */
	return _class_getIMPForSelector(cl, selector);
}

IMP
class_getMethodImplementation_stret(Class cls, SEL name)
{
	return class_getMethodImplementation(cls, name);
}



#include "MRObjectMethods.h"

#include "../os.h"
#include "../class.h"
#include "../selector.h"
#include "../utils.h"
#include "../message.h"

#pragma mark MRObject

id _C_MRObject_alloc_(id self, SEL _cmd){
	MRObject_instance_t *instance = (MRObject_instance_t*)objc_class_create_instance((Class)self);
	instance->retainCount = 1;
	return (id)instance;
}

void _C_MRObject_initialize_(id self, SEL _cmd){
	objc_debug_log("Initializing class %s\n", objc_object_get_class_inline(self)->name);
}

id _C_MRObject_new_(id self, SEL _cmd){
	static SEL alloc_SEL;
	static SEL init_SEL;
	
	if (alloc_SEL == null_selector){
		alloc_SEL = objc_selector_register("alloc", "@@:");
	}
	
	if (init_SEL == null_selector){
		init_SEL = objc_selector_register("init", "@@:");
	}
	
	self = ((id(*)(id, SEL))objc_object_lookup_impl(self, alloc_SEL))(self, alloc_SEL);
	self = ((id(*)(id, SEL))objc_object_lookup_impl(self, init_SEL))(self, init_SEL);
	
	return self;
}

id _I_MRObject_init_(MRObject_instance_t *self, SEL _cmd){
	/** Retain count is set to 1 in +alloc. */
	return (id)self;
}

id _I_MRObject_retain_(MRObject_instance_t *self, SEL _cmd){
	/** Warning, this atomic function is a GCC builtin function */
	__sync_add_and_fetch(&self->retainCount, 1);
	return (id)self;
}

void _I_MRObject_release_(MRObject_instance_t *self, SEL _cmd){
	int retain_cnt = __sync_sub_and_fetch(&self->retainCount, 1);
	if (retain_cnt == 0){
		/** Dealloc */
		static SEL dealloc_selector;
		if (dealloc_selector == null_selector){
			dealloc_selector = objc_selector_register("dealloc", "v@:");
		}
		objc_msg_send((id)self, dealloc_selector);
	}else if (retain_cnt < 0){
		objc_abort("Over-releasing an object %p!", self);
	}
}

id _C_MRObject_retain_noop_(Class self, SEL _cmd){
	return (id)self;
}

void _C_MRObject_release_noop_(Class self, SEL _cmd){
	/** No-op. */
}

void _I_MRObject_dealloc_(MRObject_instance_t *self, SEL _cmd){
	objc_object_deallocate((id)self);
}

#pragma mark -
#pragma mark __MRConstString

const char *_I___MRConstString_cString_(__MRConstString_instance_t *self, SEL _cmd){
	return self->cString;
}

unsigned int _I___MRConstString_length_(__MRConstString_instance_t *self, SEL _cmd){
	return objc_strlen(self->cString);
}

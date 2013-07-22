
#include "MRObjectMethods.h"

#include "../kernobjc/runtime.h"
#include "../private.h"
#include "../message.h"
#include "../associative.h"

#pragma mark MRObject

id _C_MRObject_alloc_(id self, SEL _cmd){
	MRObject_instance_t *instance = (MRObject_instance_t*)class_createInstance((Class)self, 0);
	instance->retainCount = 0;
	return (id)instance;
}
void _C_MRObject_load_(id self, SEL _cmd){
	objc_debug_log("+load message sent to class %s\n", object_getClassName(self));
}

void _C_MRObject_initialize_(id self, SEL _cmd){
	objc_debug_log("Initializing class %s\n", object_getClass(self)->name);
}

id _C_MRObject_new_(id self, SEL _cmd){
	static SEL alloc_SEL;
	static SEL init_SEL;
	
	if (alloc_SEL == null_selector){
		alloc_SEL = sel_registerName("alloc", "@@:");
	}
	
	if (init_SEL == null_selector){
		init_SEL = sel_registerName("init", "@@:");
	}
	
	self = objc_msgSend(self, alloc_SEL);
	self = objc_msgSend(self, init_SEL);
	
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
			dealloc_selector = sel_registerName("dealloc", "v@:");
		}
		objc_msgSend((id)self, dealloc_selector);
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
	objc_remove_associated_objects((id)self);
	object_dispose((id)self);
}

#pragma mark -
#pragma mark __MRConstString

const char *_I___MRConstString_cString_(__MRConstString_instance_t *self, SEL _cmd){
	return self->cString;
}

unsigned int _I___MRConstString_length_(__MRConstString_instance_t *self, SEL _cmd){
	return (unsigned int)strlen(self->cString);
}

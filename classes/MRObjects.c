/**
 * A basic object class with R/R methods.
 */

#include "MRObjectMethods.h"
#include "../class_registry.h"


/*************** MRObject ***************/
/** MRObject class, ivars and methods. */

#pragma mark -
#pragma mark MRObject


static struct objc_method_list_struct MRObject_class_method_list = {
	0, // Next
	4, // Size
	{
		{
			(IMP)_C_MRObject_alloc_,
			"alloc",
			"@@:",
			0
		},
		{
			(IMP)_C_MRObject_new_,
			"new",
			"@@:",
			0
		},
		{
			(IMP)_C_MRObject_initialize_,
			"initialize",
			"v@:",
			0
		},
		{
			(IMP)_C_MRObject_retain_noop_,
			"retain",
			"@@:",
			0
		},
		{
			(IMP)_C_MRObject_release_noop_,
			"release",
			"v@:",
			0
		}
	}
};

static struct objc_method_list_struct MRObject_instance_method_list = {
	0, // Next
	4, // Size
	{
		{
			(IMP)_I_MRObject_init_,
			"init",
			"@@:",
			0
		},
		{
			(IMP)_I_MRObject_retain_,
			"retain",
			"@@:",
			0
		},
		{
			(IMP)_I_MRObject_release_,
			"release",
			"v@:",
			0
		},
		{
			(IMP)_I_MRObject_dealloc_,
			"dealloc",
			"v@:",
			0
		}
	}
};

static struct objc_ivar_list_struct MRObject_ivar_list = {
	2, // Size
	{
		{
			"isa",
			"#",
			sizeof(Class),
			0,
			__alignof(Class)
		},
		{
			"retain_count",
			"i",
			sizeof(int),
			sizeof(Class),
			__alignof(int)
		}
	}
};

/**
 * Non-static to export a pointer to the classes directly.
 *
 * In particular, the MRString export allows compile-time
 * creation of static strings.
 */

struct objc_class MRObject_metaclass = {
	.isa = Nil, /** isa pointer gets connected when registering. */
	.super_class = Nil, /** Root class */
	.name = "MRObject",
	.methods = &MRObject_class_method_list, /** Class methods */
	.flags = {
		.meta = 1,
		.in_construction = 1
	}
};

static unsigned int MRObject_class_ivar_offsets[2] = { 0, 0 };

struct objc_class MRObject_class = {
	.isa = &MRObject_metaclass, /** isa pointer going to the meta class. */
	.super_class = Nil, /** Root class */
	.name = "MRObject",
	.methods = &MRObject_instance_method_list, /** Methods */
	.ivars = &MRObject_ivar_list, /** Ivars */
	.ivar_offsets = (unsigned int*)&MRObject_class_ivar_offsets,
	.flags = {
		.in_construction = 1
	}
};

#pragma mark -
#pragma mark __MRConstString

static struct objc_method_list_struct __MRConstString_instance_method_list = {
	0, // Next
	4, // Size
	{
		{
			(IMP)_C_MRObject_retain_noop_, /** We can use the no-op class implementation. */
			"retain",
			"@@:",
			0
		},
		{
			(IMP)_C_MRObject_release_noop_,
			"release",
			"v@:",
			0
		},
		{
			(IMP)_I___MRConstString_cString_,
			"cString",
			"^@:",
			0
		},
		{
			(IMP)_I___MRConstString_length_,
			"length",
			"u@:",
			0
		}
	}
};


static struct objc_ivar_list_struct __MRConstString_ivar_list = {
	1, // Size
	{
		{
			"cString",
			"^",
			sizeof(const char *),
			sizeof(MRObject_instance_t),
			__alignof(const char *)
		}
	}
};

struct objc_class __MRConstString_metaclass = {
	.isa = &MRObject_metaclass, /** isa pointer gets connected when registering. */
	.super_class = &MRObject_metaclass, /** Superclass */
	.name = "__MRConstString",
	.flags = {
		.meta = 1,
		.in_construction = 1
	}
};

static unsigned int __MRConstString_class_ivar_offsets[2] = { 0, 0 };

struct objc_class __MRConstString_class = {
	.isa = &__MRConstString_metaclass, /** isa pointer gets connected when registering. */
	.super_class = &MRObject_class, /** Superclass */
	.name = "__MRConstString",
	.methods = &__MRConstString_instance_method_list, /** Class methods */
	.ivars = &__MRConstString_ivar_list,
	.ivar_offsets = (unsigned int*)&__MRConstString_class_ivar_offsets,
	.flags = {
		.in_construction = 1
	}
};

#pragma mark -
#pragma mark Class Installer

void objc_install_base_classes(void){
	struct objc_class *classes[] = {
		&MRObject_class,
		&__MRConstString_class
	};
	
	objc_debug_log("Registering base classes with the runtime.\n");
	
	objc_class_register_classes(classes, 2);
	
}

/**
 * A basic object class with R/R methods.
 */

#include "../private.h"
#include "../types.h"

#include "MRObjectMethods.h"

/*************** MRObject ***************/
/** MRObject class, ivars and methods. */

#pragma mark -
#pragma mark MRObject

static struct objc_method_prototype _C_MRObject_alloc_mp = {
	"alloc",
	"@@:",
	(IMP)_C_MRObject_alloc_,
	0
};

static struct objc_method_prototype _C_MRObject_new_mp = {
	"new",
	"@@:",
	(IMP)_C_MRObject_new_,
	0
};

static struct objc_method_prototype _I_MRObject_init_mp = {
	"init",
	"@@:",
	(IMP)_I_MRObject_init_,
	0
};

static struct objc_method_prototype _I_MRObject_retain_mp = {
	"retain",
	"@@:",
	(IMP)_I_MRObject_retain_,
	0
};

static struct objc_method_prototype _C_MRObject_retain_mp = {
	"retain",
	"@@:",
	(IMP)_C_MRObject_retain_noop_,
	0
};

static struct objc_method_prototype _I_MRObject_release_mp = {
	"release",
	"v@:",
	(IMP)_I_MRObject_release_,
	0
};

static struct objc_method_prototype _C_MRObject_release_mp = {
	"release",
	"v@:",
	(IMP)_C_MRObject_release_noop_,
	0
};

static struct objc_method_prototype _I_MRObject_dealloc_mp = {
	"dealloc",
	"v@:",
	(IMP)_I_MRObject_dealloc_,
	0
};

static struct objc_method_prototype _I_MRObject_forwardedMethodForSelector_mp = {
	"forwardedMethodForSelector:",
	"^@::",
	(IMP)_IC_MRObject_forwardedMethodForSelector_,
	0
};

static struct objc_method_prototype _C_MRObject_forwardedMethodForSelector_mp = {
	"forwardedMethodForSelector:",
	"^@::",
	(IMP)_IC_MRObject_forwardedMethodForSelector_,
	0
};

static struct objc_method_prototype _I_MRObject_dropsUnrecognizedMessage_mp = {
	"dropsUnrecognizedMessage:",
	"B@::",
	(IMP)_IC_MRObject_dropsUnrecognizedMessage_,
	0
};

static struct objc_method_prototype _C_MRObject_dropsUnrecognizedMessage_mp = {
	"dropsUnrecognizedMessage:",
	"B@::",
	(IMP)_IC_MRObject_dropsUnrecognizedMessage_,
	0
};


static struct objc_method_prototype *MRObject_class_methods[] = {
	&_C_MRObject_alloc_mp,
	&_C_MRObject_new_mp,
	&_C_MRObject_release_mp,
	&_C_MRObject_retain_mp,
	&_C_MRObject_forwardedMethodForSelector_mp,
	&_C_MRObject_dropsUnrecognizedMessage_mp,
	NULL
};

static struct objc_method_prototype *MRObject_instance_methods[] = {
	&_I_MRObject_init_mp,
	&_I_MRObject_retain_mp,
	&_I_MRObject_release_mp,
	&_I_MRObject_dealloc_mp,
	&_I_MRObject_forwardedMethodForSelector_mp,
	&_I_MRObject_dropsUnrecognizedMessage_mp,
	NULL
};


static struct objc_ivar _MRObject_isa_ivar = {
	"isa",
	"#",
	sizeof(Class),
	0
};

static struct objc_ivar _MRObject_retain_count_ivar = {
	"retain_count",
	"i",
	sizeof(int),
	sizeof(Class)
};

static Ivar MRObject_ivars[] = {
	&_MRObject_isa_ivar,
	&_MRObject_retain_count_ivar,
	NULL
};

/**
 * Non-static to export a pointer to the classes directly.
 *
 * In particular, the MRString export allows compile-time
 * creation of static strings.
 */

struct objc_class_prototype MRObject_class = {
	NULL, /** isa pointer gets connected when registering. */
	NULL, /** Root class */
	"MRObject",
	MRObject_class_methods, /** Class methods */
	MRObject_instance_methods, /** Instance methods */
	MRObject_ivars, /** Ivars */
	NULL, /** Class cache. */
	NULL, /** Instance cache. */
	0, /** Instance size - computed from ivars. */
	0, /** Version. */
	{
		YES /** In construction. */
	},
	NULL /** Extra space */
};

#pragma mark -
#pragma mark __MRConstString

static struct objc_method_prototype _I___MRConstString_retain_mp = {
	"retain",
	"@@:",
	(IMP)_C_MRObject_retain_noop_, /** We can use the no-op class implementation. */
	0
};

static struct objc_method_prototype _I___MRConstString_release_mp = {
	"release",
	"v@:",
	(IMP)_C_MRObject_release_noop_,
	0
};

static struct objc_method_prototype _I___MRConstString_cString_mp = {
	"cString",
	"^@:",
	(IMP)_I___MRConstString_cString_,
	0
};

static struct objc_method_prototype _I___MRConstString_length_mp = {
	"length",
	"u@:",
	(IMP)_I___MRConstString_length_,
	0
};

static struct objc_method_prototype *__MRConstString_instance_methods[] = {
	&_I___MRConstString_cString_mp,
	&_I___MRConstString_release_mp,
	&_I___MRConstString_retain_mp,
	&_I___MRConstString_length_mp,
	NULL
};


static struct objc_ivar ___MRConstString_cString_ivar = {
	"cString",
	"^",
	sizeof(const char *),
	sizeof(MRObject_instance_t)
};

static Ivar __MRConstString_ivars[] = {
	&___MRConstString_cString_ivar,
	NULL
};

struct objc_class_prototype __MRConstString_class = {
	NULL, /** isa pointer gets connected when registering. */
	"MRObject", /** Superclass */
	"__MRConstString",
	NULL, /** Class methods */
	__MRConstString_instance_methods, /** Instance methods */
	__MRConstString_ivars, /** Ivars */
	NULL, /** Class cache. */
	NULL, /** Instance cache. */
	0, /** Instance size - computed from ivars. */
	0, /** Version. */
	{
		YES /** In construction. */
	},
	NULL /** Extra space. */
};

#pragma mark -
#pragma mark Class Installer

void objc_install_base_classes(void){
	struct objc_class_prototype *classes[] = {
		&MRObject_class,
		&__MRConstString_class,
		NULL
	};
	
	objc_class_register_prototypes(classes);
	
}

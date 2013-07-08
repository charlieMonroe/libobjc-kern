
#ifndef RUNTIME_TEST_CLASSES_H_
#define RUNTIME_TEST_CLASSES_H_

#include "../objc.h"
#include <stdio.h>
#include <time.h>
#include <float.h>

#if OBJC_HAS_AO_EXTENSION
	#include "../extras/ao-ext.h"
#endif

#if OBJC_HAS_CATEGORIES_EXTENSION
	#include "../extras/categs.h"
#endif

typedef struct {
	Class isa;
	id proxyObject;
	int i;
} MyClass;

static id _C_MyClass_alloc_(id self, SEL _cmd, ...){
	return objc_class_create_instance((Class)self);
}

static void _I_MyClass_increment_(MyClass *self, SEL _cmd, ...){
	++self->i;
}

static void _I_MyClass_incrementViaSettersAndGetters_(MyClass *self, SEL _cmd, ...){
	int *old_value_ptr;
	int new_value;
	Ivar i_ivar = objc_class_get_ivar(objc_object_get_class((id)self), "i");
	
	old_value_ptr = objc_object_get_variable((id)self, i_ivar);
	new_value = *old_value_ptr + 1;
	objc_object_set_variable((id)self, i_ivar, &new_value);
}

#if OBJC_HAS_AO_EXTENSION
static void _I_MyClass_incrementViaAO_(MyClass *self, SEL _cmd, ...){
	unsigned int old_value = (unsigned int)objc_object_get_associated_object((id)self, (void*)_cmd);
	objc_object_set_associated_object((id)self, (void*)_cmd, (id)(old_value + 1));
}
#endif

static Method _I_MyClass_forwardedMethod_(MyClass *self, SEL _cmd, SEL selector){
	return objc_object_lookup_method(self->proxyObject, selector);
}

static BOOL _I_MyClass_dropMessageForSelector_(id self, SEL _cmd, SEL selector){
	printf("Class %s supports message dropping - dropped call with selector %s.\n", objc_class_get_name(self->isa), objc_selector_get_name(selector));
	return YES;
}

static struct objc_method_prototype _C_MyClass_alloc_mp_ = {
	"alloc",
	"@@:",
	(IMP)_C_MyClass_alloc_,
	0
};

static struct objc_method_prototype _I_MyClass_increment_mp_ = {
	"increment",
	"v@:",
	(IMP)_I_MyClass_increment_,
	0
};

static struct objc_method_prototype _I_MyClass_incrementViaSettersAndGetters_mp_ = {
	"incrementViaSettersAndGetters",
	"v@:",
	(IMP)_I_MyClass_incrementViaSettersAndGetters_,
	0
};

#if OBJC_HAS_AO_EXTENSION
static struct objc_method_prototype _I_MyClass_incrementViaAO_mp_ = {
	"incrementViaAO",
	"v@:",
	(IMP)_I_MyClass_incrementViaAO_,
	0
};
#endif

static struct objc_method_prototype _I_MyClass_forwardedMethod_mp_ = {
	"forwardedMethodForSelector:",
	"^@::",
	(IMP)_I_MyClass_forwardedMethod_,
	0
};

static struct objc_method_prototype _I_MyClass_dropMessageForSelector_mp_ = {
	"dropMessageForSelector:",
	"B@::",
	(IMP)_I_MyClass_dropMessageForSelector_,
	0
};

static struct objc_ivar _MyClass_isa_proxyObject_ivar_ = {
	"proxyObject",
	"@",
	sizeof(id),
	sizeof(id)
};

static struct objc_ivar _MyClass_isa_i_ivar_ = {
	"i",
	"i",
	sizeof(int),
	sizeof(id) + sizeof(id)
};


static struct objc_method_prototype *_MyClass_class_methods[] = {
	&_C_MyClass_alloc_mp_,
	NULL
};

static struct objc_method_prototype *_MyClass_instance_methods[] = {
	&_I_MyClass_increment_mp_,
	&_I_MyClass_incrementViaSettersAndGetters_mp_,
#if OBJC_HAS_AO_EXTENSION
	&_I_MyClass_incrementViaAO_mp_,
#endif
	&_I_MyClass_forwardedMethod_mp_,
	&_I_MyClass_dropMessageForSelector_mp_,
	NULL
};

static Ivar _MyClass_ivars_[] = {
	&_MyClass_isa_proxyObject_ivar_,
	&_MyClass_isa_i_ivar_,
	NULL
};

static struct objc_class_prototype MyClass_class = {
	NULL, /** isa pointer gets connected when registering. */
	"MRObject", /** Superclass */
	"MyClass",
	_MyClass_class_methods, /** Class methods */
	_MyClass_instance_methods, /** Instance methods */
	_MyClass_ivars_, /** Ivars */
	NULL, /** Class cache. */
	NULL, /** Instance cache. */
	0, /** Instance size - computed from ivars. */
	0, /** Version. */
	{
		YES /** In construction. */
	},
	NULL /** Extra space. */
};

static void _I_MySubclass_increment_(MyClass *self, SEL _cmd, ...){
	/* i.e. [super _cmd]; */
	objc_super super;
	IMP super_imp;
	super.receiver = (id)self;
	super.class = objc_class_get_superclass(self->isa);
	super_imp = objc_object_lookup_impl_super(&super, _cmd);
	super_imp((id)self, _cmd);
	
	++self->i;
}

static struct objc_method_prototype _I_MySubclass_increment_mp_ = {
	"increment",
	"v@:",
	(IMP)_I_MySubclass_increment_,
	0
};

static struct objc_class_prototype MySubclass_class = {
	NULL, /** isa pointer gets connected when registering. */
	"MyClass", /** Superclass */
	"MySubclass",
	NULL, /** Class methods */
	NULL, /** Instance methods */
	NULL, /** Ivars */
	NULL, /** Class cache. */
	NULL, /** Instance cache. */
	0, /** Instance size - computed from ivars. */
	0, /** Version. */
	{
		YES /** In construction. */
	},
	NULL /** Extra space. */
};

#if OBJC_HAS_CATEGORIES_EXTENSION

static void _I_MyClassCategoryPrivate_incrementViaCategoryMethod_(MyClass *self, SEL _cmd){
	++self->i;
}

static struct objc_method_prototype _I_MyClassCategoryPrivate_incrementViaCategoryMethod_mp_ = {
	"incrementViaCategoryMethod",
	"v@:",
	(IMP)_I_MyClassCategoryPrivate_incrementViaCategoryMethod_,
	0
};

static struct objc_method_prototype *MyClass_Private_category_instance_methods[] = {
	&_I_MyClassCategoryPrivate_incrementViaCategoryMethod_mp_,
	NULL
};

static struct objc_category_prototype _MyClass_Privates_category_prototype_ = {
	"MyClass",
	"Privates",
	NULL,
	MyClass_Private_category_instance_methods
};

#endif


static void register_classes(void){
	#if OBJC_HAS_AO_EXTENSION
		objc_associated_object_register_extension();
	#endif
	#if OBJC_HAS_CATEGORIES_EXTENSION
		objc_categories_register_extension();
	#endif
	
	objc_class_register_prototype(&MyClass_class);
	objc_class_register_prototype(&MySubclass_class);
	
#if OBJC_HAS_CATEGORIES_EXTENSION
	objc_class_register_category_prototype(&_MyClass_Privates_category_prototype_);
#endif
}

static void print_method_list(Method *methods){
	while (*methods != NULL){
		printf("\t%s - %p\n", (*methods)->selector->name, (*methods)->implementation);
		++methods;
	}
}
static void print_ivar_list(Ivar *ivars){
	while (*ivars != NULL){
		printf("\t%s - %s - size: %d offset: %d\n", (*ivars)->name, (*ivars)->type, (*ivars)->size, (*ivars)->offset);
		++ivars;
	}
}

#if OBJC_HAS_CATEGORIES_EXTENSION
static void print_categories(Class cl){
	Category *categories = objc_class_get_category_list(cl);
	Category *orig_ptr = categories;
	while (*categories != NULL){
		Method *class_methods = objc_category_get_class_methods(*categories);
		Method *instance_methods = objc_category_get_instance_methods(*categories);
		
		printf("** %s - Class category methods:\n", objc_category_get_name(*categories));
		print_method_list(class_methods);
		
		printf("** %s - Instance category methods:\n", objc_category_get_name(*categories));
		print_method_list(instance_methods);
		
		objc_dealloc(class_methods);
		objc_dealloc(instance_methods);
		
		++categories;
	}
	objc_dealloc(orig_ptr);
}
#endif

static void print_class(Class cl){
	printf("******** Class %s ********\n", objc_class_get_name(cl));
	printf("**** Class methods:\n");
	print_method_list(objc_class_get_class_method_list(cl));
	printf("**** Instance methods:\n");
	print_method_list(objc_class_get_instance_method_list(cl));
	
#if OBJC_HAS_CATEGORIES_EXTENSION
	printf("**** Categories:\n");
	print_categories(cl);
#endif
	
	printf("**** Ivars:\n");
	print_ivar_list(objc_class_get_ivar_list(cl));
	
	printf("\n\n");
}
static void list_classes(void){
	Class *classes = objc_class_get_list();
	Class *orig_ptr = classes;
	while (*classes != NULL){
		print_class(*classes);
		++classes;
	}
	objc_dealloc(orig_ptr);
}

#define OBJC_INLINE_CACHING_NONE 0
#define OBJC_INLINE_CACHING_SELECTOR 1
#define OBJC_INLINE_CACHING_COMPLETE 2

#if OBJC_INLINE_CACHING == OBJC_INLINE_CACHING_NONE
#define OBJC_GET_IMP(obj, sel_name, sel_var, imp_var) {\
	sel_var = objc_selector_register(sel_name);\
	imp_var = objc_object_lookup_impl(obj, sel_var);\
}
#elif OBJC_INLINE_CACHING == OBJC_INLINE_CACHING_SELECTOR
#define OBJC_GET_IMP(obj, sel_name, sel_var, imp_var) {\
	static SEL sel_var##sel_var;\
	if (sel_var##sel_var == NULL){\
		sel_var##sel_var = objc_selector_register(sel_name);\
	}\
	sel_var = sel_var##sel_var;\
	imp_var = objc_object_lookup_impl(obj, sel_var);\
}
#elif OBJC_INLINE_CACHING == OBJC_INLINE_CACHING_COMPLETE
#define OBJC_GET_IMP(obj, sel_name, sel_var, imp_var) {\
	static SEL sel_var##sel_var;\
	static struct {\
		Method m;\
		Class isa;\
		unsigned int version;\
	} cache;\
	\
	if (sel_var##sel_var == NULL){\
		sel_var##sel_var = objc_selector_register(sel_name);\
	}\
	sel_var = sel_var##sel_var;\
	\
	if (cache.m == NULL || \
		(((id)obj)->isa != cache.isa || cache.version != cache.m->version)\
		){\
		cache.m = objc_object_lookup_method(obj, sel_var##sel_var);\
		cache.version = cache.m->version;\
		cache.isa = ((id)obj)->isa;\
		imp_var = cache.m->implementation;\
	}else{\
		imp_var = cache.m->implementation;\
	}\
}
#else
#error Unknown type of caching.
#endif


#define GENERATE_TEST(TEST_NAME, INSTANCE_CLASS_NAME, PREFLIGHT, ITERATIONS, INNER_CYCLE, CORRECTNESS_TEST) static clock_t TEST_NAME##_test(void){\
	MyClass *instance;\
	SEL alloc_selector = NULL;\
	IMP alloc_impl = NULL;\
	Class cl = Nil;\
	clock_t c1, c2;\
	int i;\
	\
	PREFLIGHT\
	\
	cl = objc_class_for_name(INSTANCE_CLASS_NAME);\
	OBJC_GET_IMP((id)cl, "alloc", alloc_selector, alloc_impl);\
	instance = (MyClass*)alloc_impl((id)cl, alloc_selector);\
	\
	c1 = clock();\
	for (i = 0; i < ITERATIONS; ++i){\
		INNER_CYCLE\
	}\
	c2 = clock();\
	\
	if (!(CORRECTNESS_TEST)){\
		printf("Correctness condition false for test " #TEST_NAME "!\n");\
		objc_abort("");\
	}\
	\
	return (c2 - c1);\
}


#define DISPATCH_ITERATIONS 10000000
#define ALLOCATION_ITERATIONS 10000000

#include "testing-common.h"


#endif /* RUNTIME_TEST_CLASSES_H_ */

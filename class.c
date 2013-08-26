/*
 * Implementation of the class-related functions.
 */

#include "os.h"
#include "kernobjc/types.h"
#include "kernobjc/class.h"
#include "kernobjc/runtime.h"
#include "types.h"
#include "utils.h"
#include "selector.h"
#include "class_registry.h"
#include "sarray2.h"
#include "runtime.h"
#include "class.h"
#include "private.h"

/*
 * Class structures are versioned. If a class prototype of a different
 * version is encountered, it is either ignored, if the version is
 */
#define OBJC_MAX_CLASS_VERSION_SUPPORTED ((unsigned int)0)


#pragma mark -
#pragma mark Small Object Classes

Class objc_small_object_classes[OBJC_SMALL_OBJECT_CLASS_COUNT];

PRIVATE BOOL
objc_register_small_object_class(Class cl, uintptr_t mask)
{
	if ((mask & OBJC_SMALL_OBJECT_MASK) != mask){
		/* Wrong mask */
		objc_debug_log("Trying to register a class with the wrong mask (%lx)",
					   mask);
		return NO;
	}
	
	if (sizeof(void*) ==  4){
		/* 32-bit address space only supports 1 class */
		if (objc_small_object_classes[0] == Nil) {
			objc_small_object_classes[0] = cl;
			return YES;
		}
		objc_debug_log("Cannot register class with mask (%lx) as there already"
					   " is a class [%s] registered with this mask.\n",
					   mask, class_getName(objc_small_object_classes[0]));
		return NO;
	}
	
	if (objc_small_object_classes[mask] == Nil){
		objc_small_object_classes[mask] = cl;
		return YES;
	}else{
		objc_debug_log("Cannot register class with mask (%lx) as there already"
					   " is a class [%s] registered with this mask.\n",
					   mask, class_getName(objc_small_object_classes[mask]));
	}
	return NO;
}


#pragma mark -
#pragma mark Private Functions


/*
 * Returns the size required for instances of class 'cl'. This
 * includes the extra space required by extensions.
 */
static inline size_t
_instance_size(Class cl)
{
	if (cl == Nil){
		return 0;
	}
	
	objc_assert(cl->flags.resolved,
		    "Getting instance size of unresolved class (%s)!\n",
		    class_getName(cl));
	
	if (cl->flags.meta){
		return sizeof(struct objc_class);
	}
	
	cl = objc_class_get_nonfake_inline(cl);
	
	return cl->instance_size;
}

/*
 * Searches for a method in a method list and returns it, or NULL 
 * if it hasn't been found.
 */
static inline Method
_lookup_method_in_method_list(objc_method_list *method_list, SEL selector)
{
	if (method_list == NULL){
		return NULL;
	}
	
	while (method_list != NULL) {
		int i;
		for (i = 0; i < method_list->size; ++i){
			Method m = &method_list->list[i];
			if (m->selector == selector){
				return m;
			}
		}
		method_list = method_list->next;
	}
	
	/* Nothing found. */
	return NULL;
}

/*
 * Searches for an instance method for a selector.
 */
static inline Method
_lookup_method(Class class, SEL selector)
{
	Method m;
	
	if (class == Nil || selector == null_selector){
		return NULL;
	}
	
	while (class != NULL){
		m = _lookup_method_in_method_list(class->methods, selector);
		if (m != NULL){
			return m;
		}
		class = class->super_class;
	}
	return NULL;
}



/*
 * Returns whether cl is a subclass of superclass_candidate.
 */
static inline BOOL
_class_is_subclass_of_class(Class cl, Class superclass_candidate){
	while (cl != Nil) {
		if (cl->super_class == superclass_candidate){
			return YES;
		}
		cl = cl->super_class;
	}
	return NO;
}

/*
 * Looks for an ivar named name in ivar_list.
 */
static inline Ivar
_ivar_named_in_ivar_list(objc_ivar_list *ivar_list, const char *name)
{
	if (ivar_list == NULL){
		return NULL;
	}
	
	for (int i = 0; i < ivar_list->size; ++i){
		Ivar var = &ivar_list->list[i];
		if (objc_strings_equal(var->name, name)){
			return var;
		}
	}
	
	return NULL;
}

/*
 * Finds an Ivar in class with name.
 */
static inline Ivar
_ivar_named(Class cl, const char *name)
{
	if (name == NULL){
		return NULL;
	}
	
	while (cl != Nil){
		Ivar var = _ivar_named_in_ivar_list(cl->ivars, name);
		if (var != NULL){
			return var;
		}
		cl = cl->super_class;
	}
	return NULL;
}

/*
 * Returns number of ivars on class cl. Used only
 * by the functions copying ivar lists.
 */
static inline unsigned int
_ivar_count(Class cl)
{
	objc_ivar_list *ivar_list = cl->ivars;
	if (ivar_list == NULL){
		return 0;
	}else{
		return ivar_list->size;
	}
}

/*
 * Copies over ivars declared on cl into list.
 */
static inline void
_ivars_copy_to_list(Class cl, Ivar *list, unsigned int max_count)
{
	unsigned int counter = 0;
	objc_ivar_list *ivar_list = cl->ivars;
	
	if (ivar_list == NULL){
		/* NULL-termination */
		list[0] = NULL;
		return;
	}
	
	for (int i = 0; i < ivar_list->size; ++i){
		if (counter >= max_count) {
			break;
		}
		
		list[counter] = &ivar_list->list[i];
		++counter;
	}
	
	/* NULL termination */
	list[max_count] = NULL;
}


/*
 * The same scenario as above, but in this case a call to the superclass.
 */
static inline Method
_lookup_method_super(struct objc_super *sup, SEL selector)
{
	if (sup == NULL || sup->receiver || sup->class == Nil){
		return NULL;
	}
	
	return _lookup_method(sup->class, selector);
}

/*
 * Calls .cxx_construct on a particular class and all the superclasses that
 * implement it.
 */
static void
call_cxx_construct_for_class(Class cls, id obj)
{
	struct objc_slot *slot;
	slot = objc_get_slot(cls, objc_cxx_construct_selector);
	
	if (NULL != slot) {
		cls = slot->owner->super_class;
		if (Nil != cls) {
			call_cxx_construct_for_class(cls, obj);
		}
		slot->implementation(obj, objc_cxx_construct_selector);
	}
}

/*
 * Calls .cxx_construct methods on all classes obj inherits from.
 */
PRIVATE void
call_cxx_construct(id obj)
{
	call_cxx_construct_for_class(objc_object_get_class_inline(obj), obj);
}


/*
 * Calls .cxx_destruct methods on all classes obj inherits from.
 */
PRIVATE void
call_cxx_destruct(id obj)
{
	/*
	 * Don't call object_getClass(), because we want to get hidden 
	 * classes too.
	 */
	Class cls = objc_object_get_class_inline(obj);
	
	while (cls) {
		objc_debug_log("Calling cxx_destruct on obj %p for class %s[%p]\n", obj,
					   class_getName(cls), cls);
		
		struct objc_slot *slot;
		slot = objc_get_slot(cls, objc_cxx_destruct_selector);
		
		if (NULL != slot) {
			objc_debug_log("\tSlot found.\n");
			cls = slot->owner->super_class;
			slot->implementation(obj, objc_cxx_destruct_selector);
		}else{
			objc_debug_log("\tSlot not found.\n");
			cls = Nil;
		}
	}
}


#pragma mark -
#pragma mark Regular lookup functions

Method
class_getMethod(Class cls, SEL selector)
{
	cls = objc_class_get_nonfake_inline(cls);
	if (cls == Nil || selector == null_selector){
		return NULL;
	}
	return _lookup_method(cls, selector);
}

Method
class_getInstanceMethod(Class cls, SEL selector)
{
	if (cls != Nil && cls->flags.meta){
		cls = (Class)objc_getClass(cls->name);
	}
	
	cls = objc_class_get_nonfake_inline(cls);
	if (cls == Nil || selector == null_selector){
		return NULL;
	}
	
	return _lookup_method(cls, selector);
}

Method
class_getInstanceMethodNonRecursive(Class cls, SEL selector)
{
	if (cls != Nil && cls->flags.meta){
		cls = (Class)objc_getClass(cls->name);
	}
	
	cls = objc_class_get_nonfake_inline(cls);
	if (cls == Nil || selector == null_selector){
		return NULL;
	}
	
	return _lookup_method_in_method_list(cls->methods, selector);
}

Method
class_getClassMethod(Class cls, SEL selector)
{
	if (cls == Nil || selector == null_selector){
		return NULL;
	}
	
	if (!cls->flags.meta){
		cls = cls->isa;
	}
	
	return _lookup_method(cls, selector);
}

#pragma mark -
#pragma mark Setting and getting class of an object

Class
object_getClass(id obj)
{
	return obj == nil ? Nil : objc_object_get_nonfake_class_inline(obj);
}

const char *
object_getClassName(id obj)
{
	if (obj == nil){
		return NULL;
	}
	return class_getName(object_getClass(obj));
}

Class
object_setClass(id obj, Class new_class)
{
	Class old_class;
	if (obj == nil){
		return Nil;
	}
	
	if ((uintptr_t)obj & OBJC_SMALL_OBJECT_MASK){
		return objc_object_get_class_inline(obj);
	}
	
	old_class = obj->isa;
	obj->isa = new_class;
	return old_class;
}

#pragma mark -
#pragma mark Object creation, copying and destruction

id
class_createInstance(Class cl, size_t extraBytes)
{
	if (cl == Nil){
		return nil;
	}
	
	if (!cl->flags.resolved){
		objc_log("Trying to create an instance of unfinished class"
			 " (%s).", cl->name);
		return nil;
	}
	
	if (cl->flags.meta){
		cl = (Class)objc_getClass(cl->name);
	}
	
	/* Check if cl is a small object class */
	if (sizeof(void *) == 4 && objc_small_object_classes[0] == cl){
		return (id)1;
	}else{
		for (uintptr_t i = 0; i < OBJC_SMALL_OBJECT_MASK; ++i){
			if (objc_small_object_classes[i] == cl){
				return (id)((i << 1) + 1);
			}
		}
	}
	
	size_t size = _instance_size(cl);
	
	id obj = (id)objc_zero_alloc(size, M_OBJECT_TYPE);
	obj->isa = cl;
	
	objc_debug_log("Created instance %p of class %s\n", obj,
		       class_getName(cl));
	
	call_cxx_construct(obj);
	
	return obj;
}

void
object_dispose(id obj)
{
	objc_debug_log("Deallocating instance %p\n", obj);
	
	if (obj == nil){
		return;
	}
	
	call_cxx_destruct(obj);
	objc_dealloc(obj, M_OBJECT_TYPE);
}

id
object_copy(id obj, size_t size)
{
	if (obj == nil){
		return nil;
	}
	
	id copy = objc_zero_alloc(size, M_OBJECT_TYPE);
	objc_copy_memory(copy, obj, size);
	
	return copy;
}

#pragma mark -
#pragma mark Information getters

BOOL
objc_class_is_resolved(Class cl)
{
	if (cl == Nil){
		return NO;
	}
	return cl->flags.resolved;
}

const char *
class_getName(Class cl)
{
	cl = objc_class_get_nonfake_inline(cl);
	if (cl == Nil){
		return "nil";
	}
	
	return cl->name;
}

Class
class_getSuperclass(Class cls)
{
	if (cls == Nil){
		return Nil;
	}
	
	if (!cls->flags.resolved){
		objc_class_resolve(cls);
	}
	return cls->super_class;
}

BOOL
class_isMetaClass(Class cls)
{
	return cls == Nil ? NO : cls->flags.meta;
}

int
class_getVersion(Class theClass){
	if (theClass == Nil){
		return 0;
	}
	return theClass->version;
}

void
class_setVersion(Class theClass, int version)
{
	if (theClass != Nil){
		theClass->version = version;
	}
}



id
objc_getMetaClass(const char *name)
{
	Class cl = (Class)objc_getClass(name);
	return cl == Nil ? nil : (id)cl->isa;
}

id
objc_getRequiredClass(const char *name)
{
	id cl = objc_getClass(name);
	if (cl == nil){
		objc_abort("Couldn't find required class %s\n",
			   name = NULL ? "[NULL]" : name);
	}
	return cl;
}

size_t
class_getInstanceSize(Class cls)
{
	return _instance_size(cls);
}

Method *
class_copyMethodList(Class cls, unsigned int *outCount)
{
	cls = objc_class_get_nonfake_inline(cls);
	if (cls == Nil){
		return NULL;
	}
	return objc_method_list_copy_list(cls->methods, outCount);
}

#pragma mark -
#pragma mark Ivar-related

BOOL
class_addIvar(Class cls, const char *name, size_t size,
	      uint8_t alignment, const char *types)
{
	Ivar variable;
	
	if (cls == Nil || name == NULL || size == 0 || types == NULL){
		return NO;
	}
	
	if (cls->flags.initialized){
		objc_log("Class %s is already initialized!\n", cls->name);
		objc_abort("Trying to add ivar (%s) to a class that is already"
			   " initialized.", name);
	}
	
	if (_ivar_named(cls, name) != NULL){
		objc_log("Class %s, or one of its superclasses already have"
			 " an ivar named %s!\n", cls->name, name);
		return NO;
	}
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	if (cls->ivars == NULL){
		cls->ivars = objc_ivar_list_create(1);
	}else{
		cls->ivars = objc_ivar_list_expand_by(cls->ivars, 1);
	}
	
	variable = &cls->ivars->list[cls->ivars->size - 1];
	variable->name = objc_strcpy(name);
	variable->type = objc_strcpy(types);
	variable->size = size;
	variable->align = alignment;
	
	long offset = cls->instance_size;
	if (alignment > 0){
		long padding = (alignment - (offset % alignment)) % alignment;
		offset = offset + padding;
	}
	variable->offset = (int)offset;
	
	cls->instance_size = offset + size;
	
	return YES;
}

Ivar
class_getInstanceVariable(Class cls, const char *name)
{
	if (cls == Nil){
		return NULL;
	}
	
	cls = objc_class_get_nonfake_inline(cls);
	return _ivar_named(cls, name);
}

Ivar *
class_copyIvarList(Class cl, unsigned int *outCount)
{
	if (cl == Nil){
		return NULL;
	}
	
	cl = objc_class_get_nonfake_inline(cl);
	unsigned int number_of_ivars = _ivar_count(cl);
	if (number_of_ivars == 0){
		if (outCount != NULL){
			*outCount = 0;
		}
		return NULL;
	}
	
	Ivar *ivars = objc_alloc(sizeof(Ivar) * number_of_ivars, M_IVAR_TYPE);
	_ivars_copy_to_list(cl, ivars, number_of_ivars);
	if (outCount != NULL){
		*outCount = number_of_ivars;
	}
	
	return ivars;
}

void *
object_getIndexedIvars(id obj)
{
	if (obj == nil){
		return NULL;
	}
	
	Class cl = object_getClass(obj);
	if (cl->instance_size == 0 || cl->flags.meta){
		return (char*)obj + sizeof(struct objc_class);
	}
	
	return (char*)obj + cl->instance_size;
}


#pragma mark -
#pragma mark Ivars

id
object_getIvar(id obj, Ivar ivar)
{
	char *var_ptr;
	
	if (obj == nil || ivar == NULL){
		return NULL;
	}
	
	var_ptr = (char*)obj;
	var_ptr += ivar->offset;
	return *(id*)var_ptr;
}

void
object_setIvar(id obj, Ivar ivar, id value)
{
	if (obj == nil || ivar == NULL){
		return;
	}

	char *var_ptr = (char*)obj;
	var_ptr += ivar->offset;
	*(id*)var_ptr = value;
}


const char *
ivar_getName(Ivar v)
{
	return v == NULL ? NULL : v->name;
}

ptrdiff_t
ivar_getOffset(Ivar v)
{
	return v == NULL ? 0 : v->offset;
}

const char *
ivar_getTypeEncoding(Ivar v)
{
	return v == NULL ? NULL : v->type;
}

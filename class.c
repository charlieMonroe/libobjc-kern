/**
 * Implementation of the class-related functions.
 */

#include "private.h"
#include "classext.h"
#include "os.h"
#include "utils.h"
#include "selector.h"
#include "method.h"

/**
 * A class holder - all classes that get registered
 * with the run-time get stored here.
 */
objc_class_holder objc_classes;

/**
 * We store the classes in an array as well for subclasses lookup, for example.
 */
objc_array objc_classes_array;

/**
 * Class extension linked list.
 */
objc_class_extension *class_extensions;

/**
 * A lock that is used for manipulating with classes - e.g. adding a class
 * to the run-time, etc.
 *
 * All actions performed with this lock locked, should be rarely performed,
 * or at least performed seldomly.
 */
objc_rw_lock objc_runtime_lock;

/**
 * A cached forwarding selectors.
 */
static SEL objc_forwarding_selector = NULL;
static SEL objc_drops_unrecognized_forwarding_selector = NULL;

/**
 * A function that is returned when the receiver is nil.
 */
static id _objc_nil_receiver_function(id self, SEL _cmd, ...){
	return nil;
}

/**
 * A method wrapper for the _objc_nil_receiver_function.
 */
static struct objc_method _objc_nil_receiver_method = {
	NULL, /** No need for a selector. */
	NULL, /** No need for types. */
	_objc_nil_receiver_function, /** Implementation. */
	0 /** Version */
};

/**
 * Class structures are versioned. If a class prototype of a different
 * version is encountered, it is either ignored, if the version is
 * 
 */
#define OBJC_MAX_CLASS_VERSION_SUPPORTED ((unsigned int)0)


#pragma mark -
#pragma mark -
#pragma mark Private Functions

/**
 * Returns the extra space needed in the class structure by
 * the class extensions.
 */
OBJC_INLINE unsigned int _extra_class_space_for_extensions(void) OBJC_ALWAYS_INLINE;
OBJC_INLINE unsigned int _extra_class_space_for_extensions(void){
	static unsigned int cached_class_result = 0;
	objc_class_extension *ext;
	
	if (cached_class_result != 0 || class_extensions == NULL){
		/* The result has already been cached, or no extensions are installed */
		return cached_class_result;
	}
	
	ext = class_extensions;
	while (ext != NULL){
		cached_class_result += ext->extra_class_space;
		ext = ext->next_extension;
	}
	
	return cached_class_result;
}

/**
 * Returns the extra space needed in each object structure by
 * the class extensions.
 */
OBJC_INLINE unsigned int _extra_object_space_for_extensions(void) OBJC_ALWAYS_INLINE;
OBJC_INLINE unsigned int _extra_object_space_for_extensions(void){
	static unsigned int cached_object_result = 0;
	objc_class_extension *ext;
	
	if (cached_object_result != 0 || class_extensions == NULL){
		/* The result has already been cached, or no extensions are installed. */
		return cached_object_result;
	}
	
	ext = class_extensions;
	while (ext != NULL){
		cached_object_result += ext->extra_object_space;
		ext = ext->next_extension;
	}
	
	return cached_object_result;
}

/**
 * Returns the size required for instances of class 'cl'. This
 * includes the extra space required by extensions.
 */
OBJC_INLINE unsigned int _instance_size(Class cl) OBJC_ALWAYS_INLINE;
OBJC_INLINE unsigned int _instance_size(Class cl){
	return cl->instance_size + _extra_object_space_for_extensions();
}

/**
 * Creates a new objc_array in place of *method_list unless it's already 
 * created. In that case, it is a no-op function.
 */
OBJC_INLINE void _initialize_method_list(objc_array *method_list) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _initialize_method_list(objc_array *method_list){
	if (*method_list != NULL){
		/* Already allocated */
		return;
	}
	
	*method_list = objc_array_create();
}

/**
 * Searches for a method in a method list and returns it, or NULL 
 * if it hasn't been found.
 */
OBJC_INLINE Method _lookup_method_in_method_list(objc_array method_list, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_method_in_method_list(objc_array method_list, SEL selector){
	objc_array_enumerator en;
	
	if (method_list == NULL){
		return NULL;
	}
	
	en = objc_array_get_enumerator(method_list);
	while (en != NULL) {
		Method *methods = en->item;
		while (*methods != NULL){
			if (objc_selectors_equal(selector, (*methods)->selector)){
				return *methods;
			}
			++methods;
		}
		en = en->next;
	}
		
	return NULL;
}

/**
 * Searches for an instance method among class extensions.
 */
OBJC_INLINE Method _lookup_extension_instance_method(Class cl, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_extension_instance_method(Class cl, SEL selector){
	Method m = NULL;
	objc_class_extension *ext;
	
	ext = class_extensions;
	while (ext != NULL){
		if (ext->instance_lookup_function != NULL){
			m = ext->instance_lookup_function(cl, selector);
			if (m != NULL){
				return m;
			}
		}
		
		ext = ext->next_extension;
	}
	
	return m;
}

/**
 * Searches for a class method among class extensions.
 */
OBJC_INLINE Method _lookup_extension_class_method(Class cl, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_extension_class_method(Class cl, SEL selector){
	Method m = NULL;
	objc_class_extension *ext;
	
	ext = class_extensions;
	while (ext != NULL){
		if (ext->class_lookup_function != NULL){
			m = ext->class_lookup_function(cl, selector);
			if (m != NULL){
				return m;
			}
		}
		
		ext = ext->next_extension;
	}
	
	return m;
}

/**
 * Looks up a method for a selector in a cache.
 */
OBJC_INLINE Method _lookup_cached_method(objc_cache cache, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_cached_method(objc_cache cache, SEL selector){
	if (cache == NULL){
		return NULL;
	}
	return objc_cache_fetch(cache, selector);
}

/**
 * Adds a method to a cache. If *cache == NULL, it gets created.
 */
OBJC_INLINE void _cache_method(objc_cache *cache, Method m) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _cache_method(objc_cache *cache, Method m){
	if (*cache == NULL){
		*cache = objc_cache_create();
	}
	objc_cache_insert(*cache, m);
}

/**
 * Searches for a class method for a selector.
 */
OBJC_INLINE Method _lookup_class_method(Class cl, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_class_method(Class cl, SEL selector){
	Method m;
	
	if (cl == Nil || selector == NULL){
		return NULL;
	}
	
	while (cl != NULL){
		m = _lookup_extension_class_method(cl, selector);
		if (m != NULL){
			/* An extension returned a valid method. */
			return m;
		}
		
		m = _lookup_method_in_method_list(cl->class_methods, selector);
		if (m != NULL){
			return m;
		}
		cl = cl->super_class;
	}
	return NULL;
}

/**
 * Looks up a class method implementation within a class. 
 * NULL if it hasn't been found, yet no-op function in case
 * the receiver is nil.
 */
OBJC_INLINE IMP _lookup_class_method_impl(Class cl, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE IMP _lookup_class_method_impl(Class cl, SEL selector){
	Method m = _lookup_cached_method(cl->class_cache, selector);
	if (m != NULL){
		return m->implementation;
	}
	
	m = _lookup_class_method(cl, selector);
	if (m == NULL){
		return NULL;
	}
	
	_cache_method(&cl->class_cache, m);
	return m->implementation;
}

/**
 * Searches for an instance method for a selector.
 */
OBJC_INLINE Method _lookup_instance_method(Class class, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_instance_method(Class class, SEL selector){
	Method m;
	
	if (class == Nil || selector == NULL){
		return NULL;
	}
	
	while (class != NULL){
		m = _lookup_extension_instance_method(class, selector);
		if (m != NULL){
			/* An extension returned a valid method. */
			return m;
		}
		
		m = _lookup_method_in_method_list(class->instance_methods, selector);
		if (m != NULL){
			return m;
		}
		class = class->super_class;
	}
	return NULL;
}

/**
 * Looks up an instance method implementation within a class.
 * NULL if it hasn't been found, yet no-op function in case
 * the receiver is nil.
 */
OBJC_INLINE IMP _lookup_instance_method_impl(Class cl, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE IMP _lookup_instance_method_impl(Class cl, SEL selector){
	Method m = _lookup_cached_method(cl->instance_cache, selector);
	if (m != NULL){
		return m->implementation;
	}
	
	m = _lookup_instance_method(cl, selector);
	if (m == NULL){
		return NULL;
	}
	
	_cache_method(&cl->instance_cache, m);
	return m->implementation;
}

/**
 * Returns whether cl is a subclass of superclass_candidate.
 */
OBJC_INLINE BOOL _class_is_subclass_of_class(Class cl, Class superclass_candidate) OBJC_ALWAYS_INLINE;
OBJC_INLINE BOOL _class_is_subclass_of_class(Class cl, Class superclass_candidate){
	while (cl != Nil) {
		if (cl->super_class == superclass_candidate){
			return YES;
		}
		cl = cl->super_class;
	}
	return NO;
}

/**
 * Flushes *cache by destroying it and creating a new one.
 */
OBJC_INLINE void _flush_cache(objc_cache *cache) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _flush_cache(objc_cache *cache){
	if (cache == NULL){
		/* This is wrong. *cache may be NULL, cache no. */
		objc_abort("Cache == NULL in _flush_cache!");
	}
	
	if (*cache != NULL){
		objc_cache old_cache = *cache;
		*cache = NULL;
		objc_cache_destroy(old_cache);
	}
}

/**
 * Adds methods from the array 'm' into the method_list. The m array doesn't
 * have to be NULL-terminated, and has to contain 'count' methods.
 *
 * The m array is copied over to be NULL-terminated, but the Method 'objects'
 * are not.
 */
OBJC_INLINE void _add_methods_to_method_list(objc_array method_list, Method *m, unsigned int count) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _add_methods_to_method_list(objc_array method_list, Method *m, unsigned int count){
	/*
	 * +1 is for the NULL termination.
	 */
	Method *methods_copy = objc_alloc((count + 1) * sizeof(Method));
	unsigned int i;
	for (i = 0; i < count; ++i){
		methods_copy[i] = m[i];
	}
	methods_copy[i] = NULL;
	
	/*
	 * Just like Apple's run-time, we do not check for duplicates.
	 * As the method list gets appended at the end, a duplicated
	 * method would simply be ignored by the run-time as the
	 * original would be found before that in the method lookup.
	 * This mechanism allows to override a function of a superclass,
	 * however.
	 */
	
	objc_array_append(method_list, methods_copy);
}

/**
 * Goes through the class list and flushes cache of each subclass of cl.
 *
 * class_methods indicates, whether to flush class-method cache, or instance-method cache.
 */
OBJC_INLINE void _flush_caches_of_subclasses_of_class(Class cl, BOOL class_methods) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _flush_caches_of_subclasses_of_class(Class cl, BOOL class_methods){
	objc_array_enumerator en = objc_array_get_enumerator(objc_classes_array);
	while (en != NULL){
		Class subclass = en->item;
		if (_class_is_subclass_of_class(subclass, cl)){
			/* The class is subclass of this class -> flush. */
			_flush_cache(class_methods ? subclass->class_cache : subclass->instance_cache);
		}
		en = en->next;
	}
	
	/* Flush cache of the class as well. */
	if (class_methods){
		objc_class_flush_class_cache(cl);
	}else{
		objc_class_flush_instance_cache(cl);
	}
}

/**
 * Goes through the methods and checks if it has been implemented by any
 * of the superclasses.
 */
OBJC_INLINE BOOL _any_method_implemented_by_superclasses(Method *m, unsigned int count, Class cl, BOOL class_methods) OBJC_ALWAYS_INLINE;
OBJC_INLINE BOOL _any_method_implemented_by_superclasses(Method *m, unsigned int count, Class cl, BOOL class_methods){
	unsigned int i;
	
	if (cl->super_class == Nil){
		/* All done, it's the root class. */
		return NO;
	}
	
	for (i = 0; i < count; ++i){
		Method method = _lookup_instance_method(cl->super_class, m[i]->selector);
		if (method != NULL){
			return YES;
		}
	}
	
	return NO;
}

/**
 * Adds class methods to class cl.
 */
OBJC_INLINE void _add_class_methods(Class cl, Method *m, unsigned int count) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _add_class_methods(Class cl, Method *m, unsigned int count){
	if (cl == NULL || m == NULL){
		return;
	}
	
	_initialize_method_list(&cl->class_methods);
	_add_methods_to_method_list(cl->class_methods, m, count);
	
	/**
	 * Unfortunately, as the cl's superclass might
	 * have implemented this method, which might
	 * have been cached by its subclasses.
	 *
	 * First, we figure out, it it really is this scenario.
	 *
	 * If it is, we need to find all subclasses and flush
	 * their caches.
	 */
	
	if (_any_method_implemented_by_superclasses(m, count, cl, YES)){
		/* Need to indeed flush caches. */
		_flush_caches_of_subclasses_of_class(cl, YES);
	}
}

/**
 * Adds instance methods to class cl.
 */
OBJC_INLINE void _add_instance_methods(Class cl, Method *m, unsigned int count) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _add_instance_methods(Class cl, Method *m, unsigned int count){
	if (cl == NULL || m == NULL){
		return;
	}
	
	_initialize_method_list(&cl->instance_methods);
	_add_methods_to_method_list(cl->instance_methods, m, count);
	
	/**
	 * Unfortunately, as the cl's superclass might
	 * have implemented this method, which might
	 * have been cached by its subclasses.
	 *
	 * First, we figure out, it it really is this scenario.
	 *
	 * If it is, we need to find all subclasses and flush
	 * their caches.
	 */
	
	if (_any_method_implemented_by_superclasses(m, count, cl, NO)){
		/* Need to indeed flush caches. */
		_flush_caches_of_subclasses_of_class(cl, NO);
	}
}

/**
 * Crashes the program because forwarding isn't supported by the class of the object.
 */
OBJC_INLINE void _forwarding_not_supported_abort(id obj, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _forwarding_not_supported_abort(id obj, SEL selector){
	/* i.e. the object doesn't respond to the
	 forwarding selector either. */
	objc_log("%s doesn't support forwarding and doesn't respond to selector %s!\n", obj->isa->name, objc_selector_get_name(selector));
	objc_abort("Class doesn't support forwarding.");
}

/**
 * The first part of the forwarding mechanism. obj is called a method
 * forwardedMethodForSelector: which should return a Method pointer
 * to the actual method to be called. If it doesn't wish to forward this 
 * particular selector, return NULL. Note that the Method doesn't need
 * to be registered anywhere, it may be a faked pointer.
 *
 * If NULL is returned, the run-time moves on to the second step:
 * asking the object whether to drop the message (a nil-receiver
 * method is returned), or whether to raise an exception.
 */
OBJC_INLINE Method _forward_method_invocation(id obj, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _forward_method_invocation(id obj, SEL selector){
	if (objc_selectors_equal(selector, objc_forwarding_selector) || objc_selectors_equal(selector, objc_drops_unrecognized_forwarding_selector)){
		/* Make sure the app really crashes and doesn't create an infinite cycle. */
		return NULL;
	}else{
		/* Forwarding. */
		IMP forwarding_imp;
		
		if (objc_forwarding_selector == NULL){
			objc_forwarding_selector = objc_selector_register("forwardedMethodForSelector:");
		}
		
		if (OBJC_OBJ_IS_CLASS(obj)){
			forwarding_imp = _lookup_class_method_impl((Class)obj, objc_forwarding_selector);
		}else{
			forwarding_imp = _lookup_instance_method_impl(obj->isa, objc_forwarding_selector);
		}
		
		if (forwarding_imp == NULL){
			objc_log("Class %s doesn't respond to selector %s.\n", obj->isa->name, objc_selector_get_name(selector));
			return NULL;
		}
		
		return ((Method(*)(id,SEL,SEL))forwarding_imp)(obj, objc_forwarding_selector, selector);
	}
}

/**
 * Second part of the forwarding mechanism. Should the unhandled
 * message be ignored (dropped)? If YES, the run-time goes on, otherwise
 * aborts.
 */
OBJC_INLINE BOOL _drops_unrecognized_message(id obj, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE BOOL _drops_unrecognized_message(id obj, SEL selector){
	if (objc_selectors_equal(selector, objc_forwarding_selector)
	    || objc_selectors_equal(selector, objc_drops_unrecognized_forwarding_selector)){
		/* Make sure the app really crashes and doesn't create an infinite cycle. */
		return NO;
	}else{
		/* Forwarding. */
		IMP drops_imp;
		
		if (objc_drops_unrecognized_forwarding_selector == NULL){
			objc_drops_unrecognized_forwarding_selector = objc_selector_register("dropsUnrecognizedMessage:");
		}
		
		if (OBJC_OBJ_IS_CLASS(obj)){
			drops_imp = _lookup_class_method_impl((Class)obj, objc_drops_unrecognized_forwarding_selector);
		}else{
			drops_imp = _lookup_instance_method_impl(obj->isa, objc_drops_unrecognized_forwarding_selector);
		}
		
		if (drops_imp == NULL){
			objc_log("Class %s doesn't implement call dropping mechanism.\n", obj->isa->name);
			return NO;
		}
		
		return ((BOOL(*)(id,SEL,SEL))drops_imp)(obj, objc_drops_unrecognized_forwarding_selector, selector);
	}
}

/**
 * Calls class extension object_initializer functions.
 */
OBJC_INLINE void _complete_object(id obj) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _complete_object(id obj){
	objc_class_extension *ext = class_extensions;
	void *obj_ext_beginning = objc_object_extensions_beginning(obj);
	while (ext != NULL){
		if (ext->object_initializer != NULL){
			ext->object_initializer(obj, (void*)((char*)obj_ext_beginning + ext->object_extra_space_offset));
		}
		ext = ext->next_extension;
	}
}

/**
 * Calls class extension object_deallocator functions.
 */
OBJC_INLINE void _finalize_object(id obj) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _finalize_object(id obj){
	objc_class_extension *ext;
	void *obj_ext_beginning = objc_object_extensions_beginning(obj);
	ext = class_extensions;
	while (ext != NULL){
		if (ext->object_destructor != NULL){
			ext->object_destructor(obj, (void*)((char*)obj_ext_beginning + ext->object_extra_space_offset));
		}
		ext = ext->next_extension;
	}
}

/**
 * Looks for an ivar named name in ivar_list.
 */
OBJC_INLINE Ivar _ivar_named_in_ivar_list(objc_array ivar_list, const char *name) OBJC_ALWAYS_INLINE;
OBJC_INLINE Ivar _ivar_named_in_ivar_list(objc_array ivar_list, const char *name){
	objc_array_enumerator en;
	
	if (ivar_list == NULL){
		return NULL;
	}
	
	en = objc_array_get_enumerator(ivar_list);
	while (en != NULL) {
		Ivar ivar = en->item;
		if (objc_strings_equal(name, ivar->name)){
			return ivar;
		}
		en = en->next;
	}
	
	return NULL;
}

/**
 * Finds an Ivar in class with name.
 */
OBJC_INLINE Ivar _ivar_named(Class cl, const char *name) OBJC_ALWAYS_INLINE;
OBJC_INLINE Ivar _ivar_named(Class cl, const char *name){
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

/**
 * Returns number of ivars on class cl. Used only
 * by the functions copying ivar lists.
 */
OBJC_INLINE unsigned int _ivar_count(Class cl) OBJC_ALWAYS_INLINE;
OBJC_INLINE unsigned int _ivar_count(Class cl){
	unsigned int count = 0;
	objc_array_enumerator en;
	objc_array ivar_list = cl->ivars;
	if (ivar_list == NULL){
		return count;
	}
	
	en = objc_array_get_enumerator(ivar_list);
	while (en != NULL) {
		++count;
		en = en->next;
	}
	return count;
}

/**
 * Copies over ivars declared on cl into list.
 */
OBJC_INLINE void _ivars_copy_to_list(Class cl, Ivar *list, unsigned int max_count) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _ivars_copy_to_list(Class cl, Ivar *list, unsigned int max_count){
	unsigned int counter = 0;
	objc_array ivar_list = cl->ivars;
	objc_array_enumerator en;
	if (ivar_list == NULL){
		/* NULL-termination */
		list[0] = NULL;
		return;
	}
	
	en = objc_array_get_enumerator(ivar_list);
	while (en != NULL && counter < max_count) {
		list[counter] = en->item;
		++counter;
		en = en->next;
	}
	
	/* NULL termination */
	list[max_count] = NULL;
}

/**
 * Looks up method. If obj is nil, returns the nil receiver method.
 *
 * If the method is not found, forwarding takes place.
 */
OBJC_INLINE Method _lookup_method(id obj, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_method(id obj, SEL selector){
	Method method = NULL;
	
	if (obj == nil){
		/** ++ to invalidate inline cache. */
		++_objc_nil_receiver_method.version;
		return &_objc_nil_receiver_method;
	}
	
	if (OBJC_OBJ_IS_CLASS(obj)){
		/* Class method */
		method = _lookup_cached_method(((Class)obj)->class_cache, selector);
		if (method != NULL){
			return method;
		}
		
		method = _lookup_class_method((Class)obj, selector);
		if (method != NULL){
			_cache_method(&((Class)obj)->class_cache, method);
		}
	}else{
		/* Instance method. */
		method = _lookup_cached_method(obj->isa->instance_cache, selector);
		if (method != NULL){
			return method;
		}
		
		method = _lookup_instance_method(obj->isa, selector);
		if (method != NULL){
			_cache_method(&obj->isa->instance_cache, method);
		}
	}
	
	if (method == NULL){
		/* Not found! Prepare for forwarding. */
		Method forwarded_method = _forward_method_invocation(obj, selector);
		if (forwarded_method != NULL){
			/** The object returned a method 
			 * that should be called instead.
			 */
			
			/** ++ to invalidate inline cache. */
			++forwarded_method->version;
			return forwarded_method;
		}
		
		if (forwarded_method == NULL && _drops_unrecognized_message(obj, selector)){
			/** ++ to invalidate inline cache. */
			++_objc_nil_receiver_method.version;
			return &_objc_nil_receiver_method;
		}
		
		_forwarding_not_supported_abort(obj, selector);
		return NULL;
	}
	
	return method;
}

/**
 * The same scenario as above, but in this case a call to the superclass.
 */
OBJC_INLINE Method _lookup_method_super(objc_super *sup, SEL selector) OBJC_ALWAYS_INLINE;
OBJC_INLINE Method _lookup_method_super(objc_super *sup, SEL selector){
	Method method;
	
	if (sup == NULL){
		return NULL;
	}
	
	if (sup->receiver == nil){
		return &_objc_nil_receiver_method;
	}
	
	if (OBJC_OBJ_IS_CLASS(sup->receiver)){
		/* Class method */
		method = _lookup_class_method(sup->class, selector);
	}else{
		/* Instance method. */
		method = _lookup_instance_method(sup->class, selector);
	}
	
	if (method == NULL){
		/* Not found! Prepare for forwarding. */
		objc_log("Called super to class %s, which doesn't implement selector %s.\n", sup->class->name, selector == NULL ? "(null)" : selector->name);
		if (_forward_method_invocation(sup->receiver, selector)){
			return &_objc_nil_receiver_method;
		}else{
			_forwarding_not_supported_abort(sup->receiver, selector);
			return NULL;
		}
	}
	
	return method;
}

/**
 * Calls the class_initializer on each extension for cl.
 */
OBJC_INLINE void _register_class_with_extensions(Class cl) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _register_class_with_extensions(Class cl){
	objc_class_extension *ext;
	char *extra_space;
	
	/* Pass the class through all extensions */
	ext = class_extensions;
	extra_space = (char*)cl->extra_space;
	if (extra_space != NULL){
		while (ext != NULL) {
			if (ext->class_initializer != NULL){
				ext->class_initializer(cl, (void*)extra_space);
			}
			extra_space += ext->extra_class_space;
			ext = ext->next_extension;
		}
	}
}

/**
 * Validates the class prototype.
 */
OBJC_INLINE BOOL _validate_prototype(struct objc_class_prototype *prototype) OBJC_ALWAYS_INLINE;
OBJC_INLINE BOOL _validate_prototype(struct objc_class_prototype *prototype){
	if (prototype->name == NULL || objc_strlen(prototype->name) == 0){
		objc_log("Trying to register a prototype of a class with NULL or empty name.\n");
		return NO;
	}
	
	if (objc_class_holder_lookup(objc_classes, prototype->name) != NULL){
		objc_log("Trying to register a prototype of class %s, but such a class has already been registered.\n", prototype->name);
		return NO;
	}
	
	if (prototype->super_class_name != NULL && objc_class_holder_lookup(objc_classes, prototype->super_class_name) == NULL){
		objc_log("Trying to register a prototype of class %s, but its superclass %s hasn't been registered yet.\n", prototype->name, prototype->super_class_name);
		return NO;
	}
	
	if (prototype->isa != NULL){
		objc_log("Trying to register a prototype of class %s that has non-NULL isa pointer.\n", prototype->name);
		return NO;
	}
	
	if (prototype->instance_cache != NULL || prototype->class_cache != NULL){
		objc_log("Trying to register a prototype of class %s that already has a non-NULL cache.\n", prototype->name);
		return NO;
	}
	
	if (prototype->version > OBJC_MAX_CLASS_VERSION_SUPPORTED){
		objc_log("Trying to register a prototype of class %s of a future version (%u).\n", prototype->name, prototype->version);
		return NO;
	}
	
	return YES;
}

/**
 * Adds an ivar to class from ivar prototype list.
 */
OBJC_INLINE void _add_ivars_from_prototype(Class cl, Ivar *ivars) OBJC_ALWAYS_INLINE;
OBJC_INLINE void _add_ivars_from_prototype(Class cl, Ivar *ivars){
	if (cl->super_class != Nil){
		cl->instance_size = cl->super_class->instance_size;
	}
	
	if (ivars == NULL){
		/** No ivars. */
		return;
	}
	
	cl->ivars = objc_array_create();
	
	while (*ivars != NULL) {
		objc_array_append(cl->ivars, *ivars);
		cl->instance_size += (*ivars)->size;
		
		++ivars;
	}
}

/**
 * Registers a prototype and returns the resulting Class.
 */
OBJC_INLINE Class _register_prototype(struct objc_class_prototype *prototype) OBJC_ALWAYS_INLINE;
OBJC_INLINE Class _register_prototype(struct objc_class_prototype *prototype){
	Class cl;
	unsigned int extra_space;
	
	/** First, validation. */
	if (!_validate_prototype(prototype)){
		return Nil;
	}
	
	if (prototype->version != OBJC_MAX_CLASS_VERSION_SUPPORTED){
		/**
		 * This is the place where in the future, if the class
		 * prototype gets modified, any compatibility transformations
		 * should be performed.
		 */
		objc_log("This run-time doesn't have implemented class prototype conversion %u -> %u.\n", prototype->version, OBJC_MAX_CLASS_VERSION_SUPPORTED);
		objc_abort("Cannot convert class prototype.\n");
	}
	
	cl = (Class)prototype;
	
	/**
	 * What needs to be done:
	 *
	 * 1) Lookup and connect superclass.
	 * 2) Connect isa.
	 * 3) Transform class method prototypes to objc_array.
	 * 4) Ditto with instance methods.
	 * 5) Add ivars and calculate instance size.
	 * 6) Allocate extra space and register with extensions.
	 * 7) Mark as not in construction.
	 * 8) Add to the class lists.
	 */
	
	if (prototype->super_class_name != NULL){
		cl->super_class = objc_class_holder_lookup(objc_classes, prototype->super_class_name);
	}
	
	cl->isa = cl;
	
	cl->class_methods = objc_method_transform_method_prototypes(prototype->class_methods);
	cl->instance_methods = objc_method_transform_method_prototypes(prototype->instance_methods);
	
	_add_ivars_from_prototype(cl, prototype->ivars);
	
	extra_space = _extra_class_space_for_extensions();
	if (extra_space != 0){
		cl->extra_space = objc_zero_alloc(extra_space);
	}else{
		cl->extra_space = NULL;
	}
	
	_register_class_with_extensions(cl);
	
	cl->flags.in_construction = NO;
	
	objc_class_holder_insert(objc_classes, cl);
	objc_array_append(objc_classes_array, cl);
	
	return cl;
}

/**
 * Returns the allocator for class. If a class extension
 * returns a valid allocator, then it is returned.
 * Otherwise objc_zero_alloc.
 */
OBJC_INLINE objc_allocator_f _allocator_for_class(Class cl, unsigned int size) OBJC_ALWAYS_INLINE;
OBJC_INLINE objc_allocator_f _allocator_for_class(Class cl, unsigned int size){
	objc_allocator_f allocator = objc_zero_alloc;
	objc_class_extension *ext;
	
	ext = class_extensions;
	while (ext != NULL) {
		objc_allocator_f ext_allocator;
		if (ext->object_allocator_for_class != NULL){
			ext_allocator = ext->object_allocator_for_class(cl, size);
			if (ext_allocator != NULL){
				allocator = ext_allocator;
				break;
			}
		}
		ext = ext->next_extension;
	}
	
	return allocator;
}


/***** PUBLIC FUNCTIONS *****/
/* Documentation in the header file. */

#pragma mark -
#pragma mark Adding methods

void objc_class_add_class_method(Class cl, Method m){
	if (cl == NULL || m == NULL){
		return;
	}
	
	_add_class_methods(cl, &m, 1);
}
void objc_class_add_class_methods(Class cl, Method *m, unsigned int count){
	_add_class_methods(cl, m, count);
}
void objc_class_add_instance_method(Class cl, Method m){
	if (cl == NULL || m == NULL){
		return;
	}
	
	_add_instance_methods(cl, &m, 1);
}
void objc_class_add_instance_methods(Class cl, Method *m, unsigned int count){
	_add_instance_methods(cl, m, count);
}

#pragma mark -
#pragma mark Replacing methods

IMP objc_class_replace_instance_method_implementation(Class cls, SEL name, IMP imp, const char *types){
	Method m;
	
	if (cls == Nil || name == NULL || imp == NULL || types == NULL){
		return NULL;
	}
	
	m = _lookup_method_in_method_list(cls->instance_methods, name);
	if (m == NULL){
		Method new_method = objc_method_create(name, types, imp);
		_add_instance_methods(cls, &new_method, 1);
		
		/**
		 * Method flushing is handled by the function adding methods.
		 */
	}else{
		m->implementation = imp;
		++m->version;
		
		/**
		 * There's no need to flush any caches as the whole
		 * Method pointer is cached -> hence the IMP
		 * pointer changes even inside the cache.
		 */
	}
	return m == NULL ? NULL : m->implementation;
}
IMP objc_class_replace_class_method_implementation(Class cls, SEL name, IMP imp, const char *types){
	Method m;
	
	if (cls == Nil || name == NULL || imp == NULL || types == NULL){
		return NULL;
	}
	
	m = _lookup_method_in_method_list(cls->class_methods, name);
	if (m == NULL){
		Method new_method = objc_method_create(name, types, imp);
		_add_class_methods(cls, &new_method, 1);
		
		/**
		 * Method flushing is handled by the function adding methods.
		 */
	}else{
		m->implementation = imp;
		++m->version;
		
		/**
		 * There's no need to flush any caches as the whole
		 * Method pointer is cached -> hence the IMP
		 * pointer changes even inside the cache.
		 */
	}
	return m == NULL ? NULL : m->implementation;
}

#pragma mark -
#pragma mark Creating classes

Class objc_class_create(Class superclass, const char *name) {
	Class newClass;
	unsigned int extra_space;
	
	/** Check if the run-time has been initialized. */
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
	if (name == NULL || *name == '\0'){
		objc_abort("Trying to create a class with NULL or empty name.");
	}
	
	if (superclass != Nil && superclass->flags.in_construction){
		/** Cannot create a subclass of an unfinished class.
		 * The reason is simple: what if the superclass added
		 * a variable after the subclass did so?
		 */
		objc_abort("Trying to create a subclass of unfinished class.");
	}
	
	objc_rw_lock_wlock(objc_runtime_lock);
	if (objc_class_holder_lookup(objc_classes, name) != NULL){
		/* i.e. a class with this name already exists */
		objc_log("A class with this name already exists (%s).\n", name);
		objc_rw_lock_unlock(objc_runtime_lock);
		return NULL;
	}
	
	newClass = (Class)(objc_alloc(sizeof(struct objc_class)));
	newClass->isa = newClass; /* A loop to self to detect class method calls. */
	newClass->super_class = superclass;
	newClass->name = objc_strcpy(name);
	newClass->class_methods = NULL; /* Lazy-loading */
	newClass->instance_methods = NULL; /* Lazy-loading */
	newClass->instance_cache = NULL;
	newClass->class_cache = NULL;
	newClass->ivars = NULL;
	
	/*
	 * The instance size needs to be 0, as the root class
	 * is responsible for adding an isa ivar.
	 */
	newClass->instance_size = 0;
	if (superclass != Nil){
		newClass->instance_size = superclass->instance_size;
	}
	
	newClass->flags.in_construction = YES;
	
	extra_space = _extra_class_space_for_extensions();
	if (extra_space != 0){
		newClass->extra_space = objc_zero_alloc(extra_space);
	}else{
		newClass->extra_space = NULL;
	}
	
	objc_class_holder_insert(objc_classes, newClass);
	objc_array_append(objc_classes_array, newClass);
	
	objc_rw_lock_unlock(objc_runtime_lock);
	
	return newClass;
}
void objc_class_finish(Class cl){
	if (cl == Nil){
		objc_abort("Cannot finish a NULL class!\n");
		return;
	}
	
	_register_class_with_extensions(cl);
	
	/* That's it! Just mark it as not in construction */
	cl->flags.in_construction = NO;
}

#pragma mark -
#pragma mark Responding to selectors

BOOL objc_class_responds_to_instance_selector(Class cl, SEL selector){
	return _lookup_instance_method(cl, selector) != NULL;
}
BOOL objc_class_responds_to_class_selector(Class cl, SEL selector){
	return _lookup_class_method(cl, selector) != NULL;
}


#pragma mark -
#pragma mark Regular lookup functions

Method objc_lookup_class_method(Class cl, SEL selector){
	return _lookup_class_method(cl, selector);
}
IMP objc_lookup_class_method_impl(Class cl, SEL selector){
	/** No forwarding here! This is simply to lookup 
	 * a method implementation.
	 */
	return _lookup_class_method_impl(cl, selector);
}
Method objc_lookup_instance_method(id obj, SEL selector){
	if (obj == nil){
		return NULL;
	}
	return _lookup_instance_method(obj->isa, selector);
}
IMP objc_lookup_instance_method_impl(id obj, SEL selector){
	if (obj == nil){
		return NULL;
	}
	return _lookup_instance_method_impl(obj->isa, selector);
}

#pragma mark -
#pragma mark Setting class of an object

Class objc_object_set_class(id obj, Class new_class){
	Class old_class;
	if (obj == nil){
		return Nil;
	}
	old_class = obj->isa;
	obj->isa = new_class;
	return old_class;
}

#pragma mark -
#pragma mark Object creation, copying and destruction

id objc_class_create_instance(Class cl){
	id obj;
	objc_allocator_f allocator;
	unsigned int size;
	
	if (cl->flags.in_construction){
		objc_log("Trying to create an instance of unfinished class (%s).", cl->name);
		return nil;
	}
	
	size = _instance_size(cl);
	allocator = _allocator_for_class(cl, size);
	
	obj = (id)allocator(size);
	obj->isa = cl;
	
	_complete_object(obj);
	
	return obj;
}
void objc_object_deallocate(id obj){
	objc_deallocator_f deallocator = objc_dealloc;
	objc_class_extension *ext;
	unsigned int size_of_obj;
	
	if (obj == nil){
		return;
	}
	
	size_of_obj = _instance_size(obj->isa) + _extra_object_space_for_extensions();
	
	_finalize_object(obj);
	
	ext = class_extensions;
	while (ext != NULL) {
		objc_deallocator_f ext_deallocator;
		if (ext->object_deallocator_for_object != NULL){
			ext_deallocator = ext->object_deallocator_for_object(obj, size_of_obj);
			if (ext_deallocator != NULL){
				deallocator = ext_deallocator;
				break;
			}
		}
		ext = ext->next_extension;
	}
	
	deallocator(obj);
}

id objc_object_copy(id obj){
	id copy;
	objc_allocator_f allocator;
	unsigned int size;
	
	if (obj == nil){
		return nil;
	}
	
	size = _instance_size(obj->isa);
	allocator = _allocator_for_class(obj->isa, size);
	
	copy = allocator(size);
	
	objc_copy_memory(obj, copy, size);
	
	return copy;
}
void objc_complete_object(id instance){
	_complete_object(instance);
}
void objc_finalize_object(id instance){
	_finalize_object(instance);
}



#pragma mark -
#pragma mark Object lookup

Method objc_object_lookup_method(id obj, SEL selector){
	return _lookup_method(obj, selector);
}
Method objc_object_lookup_method_super(objc_super *sup, SEL selector){
	return _lookup_method_super(sup, selector);
}
IMP objc_object_lookup_impl(id obj, SEL selector){
	return _lookup_method(obj, selector)->implementation;
}
IMP objc_object_lookup_impl_super(objc_super *sup, SEL selector){
	return _lookup_method_super(sup, selector)->implementation;
}

/***** INFORMATION GETTERS *****/
#pragma mark -
#pragma mark Information getters

BOOL objc_class_in_construction(Class cl){
	return cl->flags.in_construction;
}
const char *objc_class_get_name(Class cl){
	return cl->name;
}
Class objc_class_get_superclass(Class cl){
	return cl->super_class;
}
Class objc_object_get_class(id obj){
	return obj == nil ? Nil : obj->isa;
}
unsigned int objc_class_instance_size(Class cl){
	return _instance_size(cl);
}
Method *objc_class_get_class_method_list(Class cl){
	if (cl == Nil){
		return NULL;
	}
	return objc_method_list_flatten(cl->class_methods);
}
Method *objc_class_get_instance_method_list(Class cl){
	if (cl == Nil){
		return NULL;
	}
	return objc_method_list_flatten(cl->instance_methods);
}
Class *objc_class_get_list(void){
	unsigned int count = 0;
	unsigned int i = 0;
	objc_array_enumerator en;
	Class *classes;
	
	en = objc_array_get_enumerator(objc_classes_array);
	while (en != NULL){
		++count;
		en = en->next;
	}
	
	classes = (Class*)objc_alloc(sizeof(Class) * (count + 1));
	
	en = objc_array_get_enumerator(objc_classes_array);
	while (en != NULL && i < count) {
		classes[i] = en->item;
		++i;
		en = en->next;
	}
	
	/* NULL termination. */
	classes[count] = NULL;
	
	return classes;
}
Class objc_class_for_name(const char *name){
	Class c;
	
	/** Check if the run-time has been initialized. */
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
	if (name == NULL){
		return Nil;
	}
	
	c = objc_class_holder_lookup(objc_classes, name);
	if (c == NULL || c->flags.in_construction){
		/* NULL, or still in construction */
		return Nil;
	}
	
	return c;
}


/***** IVAR-RELATED *****/
#pragma mark -
#pragma mark Ivar-related

Ivar objc_class_add_ivar(Class cls, const char *name, unsigned int size, unsigned int alignment, const char *types){
	Ivar variable;
	
	if (cls == Nil || name == NULL || size == 0 || types == NULL){
		return NULL;
	}
	
	if (!cls->flags.in_construction){
		objc_log("Class %s isn't in construction!\n", cls->name);
		objc_abort("Trying to add ivar to a class that isn't in construction.");
	}
	
	if (_ivar_named(cls, name) != NULL){
		objc_log("Class %s, or one of its superclasses already have an ivar named %s!\n", cls->name, name);
		return NULL;
	}
	
	variable = (Ivar)objc_alloc(sizeof(struct objc_ivar));
	variable->name = objc_strcpy(name);
	variable->type = objc_strcpy(types);
	variable->size = size;
	
	/* The offset is the aligned end of the instance size. */
	variable->offset = cls->instance_size;
	if (alignment != 0 && (variable->offset % alignment) > 0){
		variable->offset = (variable->offset + (alignment - 1)) & ~(alignment - 1);
	}
	
	cls->instance_size = variable->offset + size;
	
	if (cls->ivars == NULL){
		cls->ivars = objc_array_create();
	}
	objc_array_append(cls->ivars, variable);
	
	return variable;
}
Ivar objc_class_get_ivar(Class cls, const char *name){
	return _ivar_named(cls, name);
}
Ivar *objc_class_get_ivar_list(Class cl){
	unsigned int number_of_ivars = _ivar_count(cl);
	Ivar *ivars = objc_alloc(sizeof(Ivar) * (number_of_ivars + 1));
	_ivars_copy_to_list(cl, ivars, number_of_ivars);
	return ivars;
}
Ivar objc_object_get_variable_named(id obj, const char *name, void **out_value){
	Ivar ivar;
	
	if (obj == nil || name == NULL || out_value == NULL){
		return NULL;
	}
	
	ivar = _ivar_named(obj->isa, name);
	if (ivar == NULL){
		return NULL;
	}
	
	objc_copy_memory((char*)obj + ivar->offset, *out_value, ivar->size);
	
	return ivar;
}
Ivar objc_object_set_variable_named(id obj, const char *name, void *value){
	Ivar ivar;
	
	if (obj == nil || name == NULL){
		return NULL;
	}
	
	ivar = _ivar_named(obj->isa, name);
	if (ivar == NULL){
		return NULL;
	}
	
	objc_copy_memory(value, (char*)obj + ivar->offset, ivar->size);
	
	return ivar;
}
void *objc_object_get_variable(id obj, Ivar ivar){
	char *var_ptr;
	
	if (obj == nil || ivar == NULL){
		return NULL;
	}
	
	var_ptr = (char*)obj;
	var_ptr += ivar->offset;
	return var_ptr;
}
void objc_object_set_variable(id obj, Ivar ivar, void *value){
	if (obj == nil || ivar == NULL){
		return;
	}
	
	objc_copy_memory(value, (char*)obj + ivar->offset, ivar->size);
}

/***** PROTOTYPE-RELATED *****/
#pragma mark -
#pragma mark Prototype-related

Class objc_class_register_prototype(struct objc_class_prototype *prototype){
	Class cl;
	
	/** Check if the run-time has been initialized. */
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
	objc_rw_lock_wlock(objc_runtime_lock);
	cl = _register_prototype(prototype);
	objc_rw_lock_unlock(objc_runtime_lock);
	return cl;
}
void objc_class_register_prototypes(struct objc_class_prototype *prototypes[]){
	unsigned int i = 0;
	
	/** Check if the run-time has been initialized. */
	if (!objc_runtime_has_been_initialized){
		objc_runtime_init();
	}
	
	objc_rw_lock_wlock(objc_runtime_lock);
	while (prototypes[i] != NULL){
		_register_prototype(prototypes[i]);
		++i;
	}
	objc_rw_lock_unlock(objc_runtime_lock);
}


/***** EXTENSION-RELATED *****/
#pragma mark -
#pragma mark Extension-related

void objc_class_add_extension(objc_class_extension *extension){
	if (objc_classes != NULL){
		objc_abort("The run-time has already been initialized."
				" No class extensions may be installed at this point anymore.");
	}
	
	if (class_extensions == NULL){
		class_extensions = extension;
	}else{
		/* Prepend the extension */
		extension->next_extension = class_extensions;
		class_extensions = extension;
	}
}

/**** CACHE-RELATED ****/
#pragma mark -
#pragma mark Cache-related

/**
 * Flushing caches is a little tricky. As the structure is
 * read-lock-free, there can be multiple readers present.
 *
 * To solve this, the structure must handle this. The default
 * implementation solves this by simply marking the structure
 * as 'to be deallocated'. At the beginning of each read from
 * the cache, reader count is increased, at the end decreased.
 * 
 * Once the reader count is zero and the structure is marked
 * as to be deallocated, it is actually deallocated. This can be
 * done simply because the cache structure is marked to be
 * deleted *after* a new one has been created and placed instead.
 */

void objc_class_flush_caches(Class cl){
	if (cl == Nil){
		return;
	}
	
	_flush_cache(&cl->class_cache);
	_flush_cache(&cl->instance_cache);
}
void objc_class_flush_instance_cache(Class cl){
	if (cl == Nil){
		return;
	}
	_flush_cache(&cl->instance_cache);
}
void objc_class_flush_class_cache(Class cl){
	if (cl == Nil){
		return;
	}
	_flush_cache(&cl->class_cache);
}

/***** INITIALIZATION *****/
#pragma mark -
#pragma mark Initializator-related

/**
 * Initializes the class extensions and internal class structures.
 */
void objc_class_init(void){
	/* Cache the extension offsets */
	objc_class_extension *ext = class_extensions;
	unsigned int class_extra_space = 0;
	unsigned int object_extra_space = 0;
	while (ext != NULL) {
		ext->class_extra_space_offset = class_extra_space;
		ext->object_extra_space_offset = object_extra_space;
		class_extra_space += ext->extra_class_space;
		object_extra_space += ext->extra_object_space;
		ext = ext->next_extension;
	}
	
	objc_runtime_lock = objc_rw_lock_create();
	
	objc_classes = objc_class_holder_create();
	objc_classes_array = objc_array_create();
}


#ifndef OBJC_CLASS_EXTENSION_H_
#define OBJC_CLASS_EXTENSION_H_

#include "types.h"
#include "os.h" /* For OBJC_INLINE */

typedef struct _objc_class_extension {
	/**
	 * A pointer to the next extension. Extensions
	 * are stored in a linked list. Do not modify this
	 * field.
	 */
	struct _objc_class_extension *next_extension;
	
	/**
	 * NOTE: any of the following functions *may* be NULL if no
	 * action is required.
	 */
	
	/**
	 * These functions may return a custom allocator
	 * and deallocator for a specific class. Note that
	 * only the first class extension that returns a valid
	 * allocator is used.
	 *
	 * The first argument is the class of the object,
	 * the second argument is the size of the instance,
	 * including the extra space required by class extensions,
	 * and possibly the extra_bytes that may be passed to 
	 * objc_class_create_instance.
	 *
	 * Note that the memory returned *should* be zero'ed
	 * out. If it is not, the run-time will not zero it.
	 */
	objc_allocator_f(*object_allocator_for_class)(Class, unsigned int);
	
	/**
	 * Warning: While the deallocator also passes the instance
	 * size as the second argument, it is the instance size of
	 * obj->isa - if the isa has been changed since the allocation
	 * of the object, it doesn't have to match. This argument
	 * is mainly to serve as a hint, where the object may have come
	 * from if you base your allocators on the instance size.
	 */
	objc_deallocator_f(*object_deallocator_for_object)(id, unsigned int);
	
	/**
	 * This function is responsible for initializing
	 * the extra space in the class. The second parameter
	 * is a pointer to the extra space within the class
	 * structure.
	 */
	void(*class_initializer)(Class, void*);
	
	/**
	 * This function is responsible for initializing
	 * the extra space in an object. The second parameter
	 * is a pointer to the extra space within the object
	 * structure.
	 */
	void(*object_initializer)(id, void*);
	
	/**
	 * This function is responsible for deallocating
	 * any dynamically allocated space within the object
	 * extra space.
	 */
	void(*object_destructor)(id, void*);
	
	
	/**
	 * These are functions that allows to modify the method
	 * implementation lookup. 
	 */
	Method(*instance_lookup_function)(Class, SEL);
	Method(*class_lookup_function)(Class, SEL);
	
	/**
	 * Extra space in the class and object structures requested.
	 * If the target architecture requires any alignments,
	 * the size must be so that a pointer following
	 * the extra space is valid.
	 */
	unsigned int extra_class_space;
	unsigned int extra_object_space;
	
	/**
	 * When the run-time is initialized, each extension's
	 * structure gets a pre-computed offsets from the extra
	 * space part of the object or class. This is merely a convenience
	 * so that the class extension list doens't need to be iterated
	 * every time an extension needs to access the object's extra
	 * space.
	 *
	 * Note though, that this is the offset *after* the end of the
	 * object variables.
	 */
	unsigned int class_extra_space_offset;
	unsigned int object_extra_space_offset;
} objc_class_extension;

/* Caller is responsible for keeping the structure in memory. */
extern void objc_class_add_extension(objc_class_extension *extension);

/**
 * Returns the beginning of the memory after the internal structure fields
 * of either a class or an object.
 *
 * Generally 
 * 
 * (objc_class_extensions_beginning(cl) + ext.class_extra_space_offset)
 *
 * is the pointer to the place where the extension's extra space starts.
 * Replace 'class' with 'object' to get the extra space for an object.
 */
OBJC_INLINE void *objc_class_extensions_beginning(Class cl) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *objc_class_extensions_beginning(Class cl){
	if (cl == Nil){
		return NULL;
	}
	return cl->extra_space;
}

OBJC_INLINE void *objc_object_extensions_beginning(id obj) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *objc_object_extensions_beginning(id obj){
	if (obj == nil){
		return NULL;
	}
	return (void*)((char*)obj + obj->isa->instance_size);
}

/**
 * The same as the functions above, only returns the space
 * where the extension begins its space.
 */
OBJC_INLINE void *objc_class_extensions_beginning_for_extension(Class cl, objc_class_extension *ext) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *objc_class_extensions_beginning_for_extension(Class cl, objc_class_extension *ext){
	if (cl == Nil){
		return NULL;
	}
	return (char*)objc_class_extensions_beginning(cl) + ext->class_extra_space_offset;
}

OBJC_INLINE void *objc_object_extensions_beginning_for_extension(id obj, objc_class_extension *ext) OBJC_ALWAYS_INLINE;
OBJC_INLINE void *objc_object_extensions_beginning_for_extension(id obj, objc_class_extension *ext){
	if (obj == nil){
		return NULL;
	}
	return (char*)objc_object_extensions_beginning(obj) + ext->object_extra_space_offset;
}


#endif /* OBJC_CLASS_EXTENSION_H_ */

/*
 * This header file contains declarations
 * of functions that deal with the class structure.
 */

#ifndef OBJC_CLASS_H_	 
#define OBJC_CLASS_H_

#include "types.h" /* For Class, BOOL, Method, ... definitions. */
#include "kernobjc/runtime.h"


#define OBJC_SMALL_OBJECT_MASK ((sizeof(void*) == 4) ? 1 : 7)
#define OBJC_SMALL_OBJECT_CLASS_COUNT ((sizeof(void*) == 4) ? 1 : 4)

Class objc_small_object_classes[OBJC_SMALL_OBJECT_CLASS_COUNT];


#pragma mark -
#pragma mark Information getters

/*
 * Returns YES if the class is resolved.
 * This generally means that objc_class_finish has been
 * called with this class, or the class was loaded from
 * an image and then resolved.
 */
BOOL	objc_class_is_resolved(Class cl);

#pragma mark - 
#pragma mark Small Object Classes

static inline Class
objc_class_for_small_object(id obj)
{
	uintptr_t mask = ((uintptr_t)obj & OBJC_SMALL_OBJECT_MASK);
	if (UNLIKELY(mask != 0)){
		if (sizeof(void*) == 4){
			/* 32-bit system */
			return objc_small_object_classes[0];
		}else{
			return objc_small_object_classes[mask];
		}
	}
	return Nil;
}

static inline BOOL
objc_object_is_small_object(id obj)
{
	return objc_class_for_small_object(obj) != Nil;
}

__attribute__((always_inline)) static inline Class
objc_object_get_class_inline(id obj)
{
	Class cl = objc_class_for_small_object(obj);
	if (UNLIKELY(cl != Nil)){
		return cl;
	}
	
	return obj->isa;
}

__attribute__((always_inline)) static inline Class
objc_class_get_nonfake_inline(Class cl)
{
	while (cl != Nil && cl->flags.fake) {
		cl = cl->super_class;
	}
	return cl;
}

__attribute__((always_inline)) static inline Class
objc_object_get_nonfake_class_inline(id obj)
{
	Class cl = objc_object_get_class_inline(obj);
	cl = objc_class_get_nonfake_inline(cl);
	return cl;
}

#endif /* !OBJC_CLASS_H_ */

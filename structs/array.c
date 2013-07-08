

#include "array.h"
#include "../os.h"

#if !OBJC_USES_INLINE_FUNCTIONS

/* Internal representation of objc_array */
typedef struct _array {
	objc_rw_lock _lock;
	objc_array_enumerator first;
	objc_array_enumerator last;
} *_array;

static void _array_destroy_enumerator(objc_array_enumerator enumerator){
	if (enumerator == NULL){
		return;
	}
	
	_array_destroy_enumerator(enumerator->next);
	objc_dealloc(enumerator);
}

objc_array array_create(void){
	_array arr = objc_alloc(sizeof(struct _array));
	arr->_lock = objc_rw_lock_create();
	arr->first = NULL;
	return arr;
}
void array_destroy(objc_array array){
	_array arr = (_array)array;
	if (arr == NULL){
		return;
	}
	
	_array_destroy_enumerator(arr->first);
	objc_rw_lock_destroy(arr->_lock);
	objc_dealloc(array);
}

void array_add(objc_array array, void *ptr){
	_array arr = (_array)array;
	objc_array_enumerator en;
	if (arr == NULL){
		return;
	}
	
	en = objc_alloc(sizeof(struct _objc_array_enumerator));
	en->item = ptr;
	en->next = NULL;
	
	objc_rw_lock_wlock(arr->_lock);
	if (arr->first == NULL){
		arr->first = arr->last = en;
	}else{
		arr->last->next = en;
		arr->last = en;
	}
	objc_rw_lock_unlock(arr->_lock);
}

objc_array_enumerator array_get_enumerator(objc_array array){
	return ((_array)array)->first;
}

#endif /* OBJC_USES_INLINE_FUNCTIONS */

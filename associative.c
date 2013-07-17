#include "associative.h"
#include "class_extra.h"
#include "arc.h"
#include "class.h"
#include "selector.h"
#include "dtable.h"

#define REF_CNT 10

typedef struct {
	void *key;
	void *value;
	
	objc_association_policy policy;
} objc_associative_reference;

typedef struct objc_object_ref_list_struct {
	struct objc_object_ref_list_struct *next;
	
	objc_rw_lock lock; // TODO make mutex from the pool instead
	
	objc_associative_reference refs[REF_CNT];
} objc_object_ref_list;



/**
 * Associated objects install a fake class for each object that uses them.
 * 
 * The class isn't an actual class, but a fake class.
 */
typedef struct {
	Class isa;
	Class super_class;
	void *dtable;
	objc_class_flags flags;
	objc_object_ref_list list;
} objc_fake_class;


/**
 * Looks for the first fake class in the class hierarchy.
 */
Class _objc_find_class_for_object(id object){
	Class cl = object->isa;
	while (cl != Nil && cl->flags.fake) {
		cl = cl->super_class;
	}
	
	return cl;
}

/**
 * Looks for the fake class in the class hierarchy. If not found
 * and create == YES, allocates it and installs the isa pointer.
 */
Class _objc_class_for_object(id object, BOOL create){
	Class cl = _objc_find_class_for_object(object);
	if (cl == Nil && create){
		// TODO locking
		cl = objc_zero_alloc(sizeof(objc_fake_class));
		cl->isa = object->isa->isa;
		cl->super_class = object->isa;
		cl->dtable = uninstalled_dtable;
		cl->flags.fake = YES;
		
		// TODO install cxx_destruct
		
		objc_rw_lock_init(&((objc_fake_class*)cl)->list.lock);
		
		object->isa = cl;
	}
	return cl;
}

/**
 * Returns a reference to the objc_object_ref_list struct for
 * that particular object, or NULL if none exists and create == NULL.
 */
objc_object_ref_list *_objc_ref_list_for_object(id object, BOOL create){
	if (object->isa->flags.meta){
		// It's a class - install the associated object directly onto the metaclass
		Class cl = object->isa;
		void **extra_space = objc_class_extra_with_identifier(cl, OBJC_ASSOCIATED_OBJECTS_IDENTIFIER);
		if (*extra_space == NULL && create){
			objc_object_ref_list *list = objc_zero_alloc(sizeof(objc_object_ref_list));
			objc_rw_lock_init(&list->lock);
			
			*extra_space = list;
		}
		return *extra_space;
	}
	
	// It's not a class, create a fake class and update the isa pointer
	// (if create == YES)
	Class cl = _objc_class_for_object(object, create);
	if (cl != Nil){
		return &((objc_fake_class*)cl)->list;
	}
	return NULL;
}

/**
 * Returns YES when the policy is one of the atomic ones.
 */
static inline BOOL _objc_is_policy_atomic(objc_association_policy policy){
	static objc_association_policy const OBJC_ASSOCIATION_ATOMIC = 0x300;
	return (policy & OBJC_ASSOCIATION_ATOMIC) == OBJC_ASSOCIATION_ATOMIC;
}

/*
 * Disposes of the object according to the policy. This pretty much means 
 * releasing the object unless the policy was assign.
 */
static inline void _objc_dispose_of_object_according_to_policy(id object, objc_association_policy policy){
	if (policy == OBJC_ASSOCIATION_WEAK_REF){
		void **address = (void**)object;
		if (address != NULL){
			*(void**)address = NULL;
		}
	}else if (policy != OBJC_ASSOCIATION_ASSIGN){
		objc_release(object);
	}
}

static inline objc_associative_reference *_objc_find_key_reference_in_list(objc_object_ref_list *list, void *key){
	while (list != NULL){
		for (int i = 0; i < REF_CNT; ++i){
			if (list->refs[i].key == key){
				return &list->refs[i];
			}
		}
		list = list->next;
	}
	return NULL;
}

static inline objc_associative_reference *_objc_find_free_reference_in_list(objc_object_ref_list *list, BOOL create){
	while (list != NULL){
		for (int i = 0; i < REF_CNT; ++i){
			if (list->refs[i].key == NULL){
				return &list->refs[i];
			}
		}
		
		/**
		 * Haven't found any free ref field and we're at the end of the
		 * linked list - if creation is allowed, allocate.
		 */
		if (list->next == NULL && create){
			list->next = objc_zero_alloc(sizeof(objc_object_ref_list));
			return &list->next->refs[0];
		}
		
		list = list->next;
	}
	return NULL;
}

static void _objc_remove_associative_list(objc_object_ref_list *prev, objc_object_ref_list *list, objc_rw_lock *lock, BOOL free){
	if (list->next != NULL){
		_objc_remove_associative_list(list, list->next, lock, free);
	}
	
	/**
	 * Need to lock it for the pointer reassignement.
	 */
	// TODO spinlock
	if (prev != NULL){
		objc_rw_lock_wlock(lock);
		prev->next = list->next;
		objc_rw_lock_unlock(lock);
	}
	
	for (int i = 0; i < REF_CNT; ++i){
		objc_associative_reference *ref = &list->refs[i];
		if (ref->key != NULL){
			_objc_dispose_of_object_according_to_policy(ref->value, ref->policy);
		}
		ref->key = NULL;
		ref->value = NULL;
		ref->policy = 0;
	}
	
	if (free){
		objc_dealloc(list);
	}
}

static inline void _objc_remove_associative_lists_for_object(id object){
	if (object->isa->flags.meta){
		void **extra_space = objc_class_extra_with_identifier(object->isa, OBJC_ASSOCIATED_OBJECTS_IDENTIFIER);
		if (*extra_space == NULL){
			return;
		}
		
		objc_object_ref_list *list = *extra_space;
		if (list->next != NULL){
			_objc_remove_associative_list(list, list->next, &list->lock, NO);
		}
		_objc_remove_associative_list(NULL, list, &list->lock, NO);
	}else{
		objc_fake_class *cl = (objc_fake_class*)_objc_class_for_object(object, NO);
		if (cl == (objc_fake_class*)Nil){
			return;
		}
		
		if (cl->list.next != NULL){
			_objc_remove_associative_list(&cl->list, cl->list.next, &cl->list.lock, YES);
		}
		_objc_remove_associative_list(NULL, &cl->list, &cl->list.lock, YES);
	}
}



#pragma mark -
#pragma mark Public Functions

id objc_get_associated_object(id object, void *key){
	if (object == nil || key == NULL){
		return nil;
	}
	
	if (objc_object_is_small_object(object)){
		return nil;
	}
	
	id result = nil;
	
	objc_object_ref_list *list = _objc_ref_list_for_object(object, NO);
	if (list == NULL){
		return nil;
	}
	
	objc_rw_lock_rlock(&list->lock);
	objc_associative_reference *ref = _objc_find_key_reference_in_list(list, key);
	objc_rw_lock_unlock(&list->lock);
	
	
	if (ref != NULL){
		switch (ref->policy) {
			case OBJC_ASSOCIATION_ASSIGN:
				result = ref->value;
				break;
			case OBJC_ASSOCIATION_RETAIN_NONATOMIC:
				result = objc_retain(ref->value);
				break;
			case OBJC_ASSOCIATION_COPY_NONATOMIC:
				result = objc_copy(ref->value);
				break;
			case OBJC_ASSOCIATION_COPY:
				// This is atomic, need to lock the WLOCK
				objc_rw_lock_wlock(&list->lock);
				result = objc_copy(ref->value);
				objc_rw_lock_unlock(&list->lock);
				break;
			case OBJC_ASSOCIATION_RETAIN:
				// This is atomic, need to lock the WLOCK
				objc_rw_lock_wlock(&list->lock);
				result = objc_retain(ref->value);
				objc_rw_lock_unlock(&list->lock);
				break;
			default:
				break;
		}
	}
	
	return result;
}

void objc_set_associated_object(id object, void *key, id value, objc_association_policy policy){
	if (object == nil || key == NULL){
		return;
	}
	
	if (objc_object_is_small_object(object)){
		return;
	}
	
	objc_object_ref_list *list = _objc_ref_list_for_object(object, YES);
	
	
	// WLOCK because we may be actually modifying the linked list
	objc_rw_lock_wlock(&list->lock);
	objc_associative_reference *ref = _objc_find_key_reference_in_list(list, key);
	if (ref == NULL){
		// No reference, perhaps need to allocate some
		ref = _objc_find_free_reference_in_list(list, YES);
		objc_assert(ref != NULL, "_objc_reference_create_for_object hasn't"
			    " created reference list for object %p!\n", object);
	}
	
	
	/**
	 * See whether either policy is atomic and if not, we can unlock the lock.
	 * Otherwise, keep it locked and unlock it after all the modification of ref.
	 */
	BOOL either_policy_atomic = _objc_is_policy_atomic(policy)
								|| _objc_is_policy_atomic(ref->policy);
	if (!either_policy_atomic){
		objc_rw_lock_unlock(&list->lock);
	}
	
	switch (policy) {
		case OBJC_ASSOCIATION_RETAIN_NONATOMIC:
		case OBJC_ASSOCIATION_RETAIN:
			value = objc_retain(value);
			break;
		case OBJC_ASSOCIATION_COPY_NONATOMIC:
		case OBJC_ASSOCIATION_COPY:
			value = objc_copy(value);
			break;
		default:
			break;
	}
	
	
	objc_association_policy old_policy = ref->policy;
	id old_value = ref->value;
	
	ref->key = key;
	ref->policy = policy;
	ref->value = value;
	
	_objc_dispose_of_object_according_to_policy(old_value, old_policy);
	
	
	/**
	 * If either of the policies was atomic, the lock is still locked, need
	 * to unlock it.
	 */
	if (either_policy_atomic){
		objc_rw_lock_unlock(&list->lock);
	}
	
}

void objc_remove_associated_objects(id object){
	if (object == nil){
		return;
	}
	
	if (objc_object_is_small_object(object)){
		return;
	}
	
	_objc_remove_associative_lists_for_object(object);
}

void objc_remove_associated_weak_refs(id object){
	
	if (objc_object_is_small_object(object)){
		// Small objects do not have associated objects
		return;
	}
	
	/**
	 * This function is quite similar to the objc_remove_associated_objects,
	 * but doesn't actually free any objects, or any of the objc_object_ref_list
	 * structures.
	 */
	
	objc_object_ref_list *list = _objc_ref_list_for_object(object, NO);
	if (list == NULL){
		// Nothing to do
		return;
	}
	
	/**
	 * Go through the lists and remove all weak refs.
	 */
	
	objc_rw_lock_wlock(&list->lock);
	
	while (list != NULL) {
		for (int i = 0; i < REF_CNT; ++i){
			objc_associative_reference *ref = &list->refs[i];
			if (ref->policy == OBJC_ASSOCIATION_WEAK_REF){
				if (ref->value != NULL){
					*(void**)ref->value = NULL;
				}
				ref->value = nil;
				ref->key = NULL;
				ref->policy = 0;
			}
		}
		list = list->next;
	}
	
	objc_rw_lock_unlock(&list->lock);
}


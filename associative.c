#include "associative.h"
#include "class_extra.h"
#include "arc.h"

/**
 * This implementation of associated objects makes a few assumptions about the
 * usage of associated objects.
 *
 * First of all, it assumes that it is not a widely used feature, since it does
 * not use a hash map, but rather a linked list on a per-class basis.
 *
 * It installs a class extra (see class_extra.h) with the 'aobj' identifier and
 * the data field of the extra is a linked list of the objc_object_ref_list
 * structure pointers.
 *
 * All locking is global, which is implied by the assumption about infrequent 
 * usage of associated objects.
 */


/**
 * The reference structure takes 3 pointers (objc_association_policy is a
 * typedef'd uintptr_t, which is 12 bytes on a 32-bit computer, 24 on a 64-bit.
 *
 * The next and obj pointers are 2 pointers - 8 bytes on 32-bit, 16 bytes on 64-
 * bit. With REF_CNT 10, the entire structure goes to 128 bytes on a 32-bit CPU,
 * 256 bytes on a 64-bit. If it seems like a too small of a number, try
 * REF_CNT 42, which gives 512 bytes on 32-bit CPU, 1024 on a 64-bit.
 */
#define REF_CNT 10

typedef struct {
	void *key;
	void *value;
	
	objc_association_policy policy;
} objc_associative_reference;

typedef struct objc_object_ref_list_struct {
	struct objc_object_ref_list_struct *next;
	
	id obj;
	
	objc_associative_reference refs[REF_CNT];
} objc_object_ref_list;


objc_rw_lock objc_associated_objects_lock;


static inline objc_associative_reference *_objc_reference_in_list(objc_object_ref_list *assoc, void *key){
	for (int i = 0; i < REF_CNT; ++i){
		if (assoc->refs[i].key == key){
			return &assoc->refs[i];
		}
	}
	return NULL;
}

/**
 * Returns reference to the objc_associative_reference structure. Does no
 * locking, all locking must be done by callers. Returns NULL if no such 
 * reference structure was found.
 */
static inline objc_associative_reference *_objc_reference_for_object_and_key(id object, void *key){
	Class cl = object->isa;
	void **extra_space = objc_class_extra_with_identifier(cl, OBJC_ASSOCIATED_OBJECTS_IDENTIFIER);
	if (*extra_space != NULL){
		objc_object_ref_list *assoc = *extra_space;
		while (assoc != NULL){
			if (assoc->obj == object){
				objc_associative_reference *ref = _objc_reference_in_list(assoc, key);
				if (ref != NULL){
					return ref;
				}
			}
			
			assoc = assoc->next;
		}
	}
	return NULL;
}

static inline objc_associative_reference *_objc_reference_free_in_list(objc_object_ref_list *assoc){
	// We're looking for a free spot, which really means that the spot
	// has a NULL key
	return _objc_reference_in_list(assoc, NULL);
}


/**
 * Returns a references to a free associative reference structure. If none are
 * available, allocates some. Does not acquire any locking, caller is responsible.
 * Doesn't return NULL.
 */
static inline objc_associative_reference *_objc_reference_create_for_object(id object){
	Class cl = object->isa;
	void **extra_space = objc_class_extra_with_identifier(cl, OBJC_ASSOCIATED_OBJECTS_IDENTIFIER);
	
	objc_object_ref_list *assoc = NULL;
	objc_associative_reference *ref = NULL;
	
	if (*extra_space != NULL){
		// Already some associated objects on this class
		assoc = *extra_space;
		while (assoc != NULL) {
			if (assoc->obj == object){
				ref = _objc_reference_free_in_list(assoc);
				if (ref != NULL){
					// Found a free one!
					break;
				}
			}
			assoc = assoc->next;
		}
	}
	
	if (ref == NULL){
		/*
		 * Either all of the refs slots are filled, or this object has nothing
		 * associated yet. We now allocate the objc_object_ref_list structure
		 * and prepend it as it's faster and we may assume recently used objects
		 * will be used again. We can afford to mess with the list since the
		 * WLOCK should be locked.
		 */
		
		assoc = objc_zero_alloc(sizeof(objc_object_ref_list));
		assoc->obj = object;
		assoc->next = *extra_space;
		*extra_space = assoc;
		
		ref = &assoc->refs[0];
	}
	
	return ref;
}

/**
 * Returns YES when the policy is one of the atomic ones.
 */
static inline BOOL _objc_is_policy_atomic(objc_association_policy policy){
	static objc_association_policy const OBJC_ASSOCIATION_ATOMIC = 0x300;
	return (policy & OBJC_ASSOCIATION_ATOMIC) == OBJC_ASSOCIATION_ATOMIC;
}

/**
 * Disposes of the object according to the policy. This pretty much means 
 * releasing the object unless the policy was assign.
 */
static inline void _objc_dispose_of_object_according_to_policy(id object, objc_association_policy policy){
	if (policy != OBJC_ASSOCIATION_ASSIGN){
		objc_release(object);
	}
}

/**
 * This function finds and removes a single objc_object_ref_list structure
 * from the linked list, disposes of the objects in the refs structures according
 * to the policies and frees the objc_object_ref_list itself.
 *
 * Note that the lock *must* be unlocked before disposing the values in the refs,
 * since disposing of the values might release some, leading to possible
 * deallocs and hence to other calls of the method.
 */
static inline BOOL _objc_remove_associative_list_for_object(id object){
	objc_rw_lock_wlock(&objc_associated_objects_lock);

	Class cl = object->isa;
	void **extra_space = objc_class_extra_with_identifier(cl, OBJC_ASSOCIATED_OBJECTS_IDENTIFIER);
	if (*extra_space != NULL){
		objc_object_ref_list *prev = NULL;
		objc_object_ref_list *assoc = *extra_space;
		while (assoc != NULL) {
			if (assoc->obj == object){
				// This list belongs to the object, we need to remove it
				if (prev != NULL){
					prev->next = assoc->next;
				}else{
					// It is the first item on the list
					*extra_space = assoc->next;
				}
				
				/** It has been taken out of the linked list, so now we unlock
				 * the lock and dispose of the objects in the ref list.
				 */
				objc_rw_lock_unlock(&objc_associated_objects_lock);
				
				for (int i = 0; i < REF_CNT; ++i){
					objc_associative_reference *ref = &assoc->refs[i];
					_objc_dispose_of_object_according_to_policy(ref->value, ref->policy);
				}
				
				objc_dealloc(assoc);
				
				return YES;
			}
			prev = assoc;
			assoc = assoc->next;
		}
	}

	objc_rw_lock_unlock(&objc_associated_objects_lock);
	return NO;
}


#pragma mark -
#pragma mark Public Functions

id objc_get_associated_object(id object, void *key){
	if (object == nil || key == NULL){
		return nil;
	}
	
	// TODO small objects
	
	id result = nil;
	
	objc_rw_lock_rlock(&objc_associated_objects_lock);
	
	objc_associative_reference *ref = _objc_reference_for_object_and_key(object, key);
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
				// Will be unlocked later on
				objc_rw_lock_wlock(&objc_associated_objects_lock);
				result = objc_copy(ref->value);
				break;
			case OBJC_ASSOCIATION_RETAIN:
				// This is atomic, need to lock the WLOCK
				// Will be unlocked later on
				objc_rw_lock_wlock(&objc_associated_objects_lock);
				result = objc_retain(ref->value);
				break;
			default:
				break;
		}
	}
	
	objc_rw_lock_unlock(&objc_associated_objects_lock);
	
	return result;
}

void objc_set_associated_object(id object, void *key, id value, objc_association_policy policy){
	if (object == nil || key == NULL){
		return;
	}
	
	// WLOCK because we may be actually modifying the linked list
	objc_rw_lock_wlock(&objc_associated_objects_lock);
	
	// Get the reference, or create it
	objc_associative_reference *ref = _objc_reference_for_object_and_key(object, key);
	if (ref == NULL){
		// No reference, perhaps need to allocate some
		ref = _objc_reference_create_for_object(object);
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
		objc_rw_lock_unlock(&objc_associated_objects_lock);
	}
	
	/**
	 *
	 */
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
		objc_rw_lock_unlock(&objc_associated_objects_lock);
	}
	
}

void objc_remove_associated_objects(id object){
	if (object == nil){
		return;
	}
	
	// TODO small objects
	
	/**
	 * The issue here is that we cannot simply WLOCK the lock and then iterate
	 * through the linked list removing all the lists associated with the object
	 * since we may be releasing objects according to the policy, which may lead
	 * to deallocs, which may lead to a recursion.
	 *
	 * Hence we remove a list by list. The _objc_remove_associative_list_for_object
	 * works like this: WLOCKs, finds a list associated with the object, removes
	 * it from the linked list, *UNLOCKS* and then disposes of the values 
	 * accordingly.
	 */
	while (_objc_remove_associative_list_for_object(object)){
		/* */
	}
	
}


PRIVATE void objc_associated_objects_init(void){
	objc_rw_lock_init(&objc_associated_objects_lock);
}


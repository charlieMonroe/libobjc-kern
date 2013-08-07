#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "associative.h"
#include "class_extra.h"
#include "kernobjc/arc.h"
#include "kernobjc/class.h"
#include "kernobjc/runtime.h"
#include "class.h"
#include "sarray2.h"
#include "dtable.h"
#include "spinlock.h"
#include "selector.h"
#include "utils.h"

#define REF_CNT 10

struct reference {
	void	*key;
	void	*value;
	
	objc_AssociationPolicy policy;
};

struct reference_list {
	struct reference_list	*next;
	
	objc_rw_lock		lock;
	
	struct reference	refs[REF_CNT];
};

/*
 * Associated objects install a fake class for each object that uses them.
 * 
 * The class isn't an actual class, but a fake class.
 */
struct objc_assoc_fake_class {
	OBJC_CLASS_COMMON_FIELDS;
	
	/* Reference list follows. */
	struct reference_list	list;
};


/*
 * Looks for the first fake class in the class hierarchy.
 */
static Class
_objc_find_class_for_object(id object)
{
	Class cl = object->isa;
  objc_debug_log("Looking for a class for object (%p), obj->isa = %p\n", object, cl);
	while (cl != Nil && !cl->flags.fake) {
    objc_debug_log("Getting a superclass of %p[%s]\n", cl, class_getName(cl));
		cl = class_getSuperclass(cl);
	}
	
	return cl;
}

static void
_objc_associated_object_cxx_destruct(id self, SEL _cmd)
{
	struct objc_assoc_fake_class *cl;
	cl = (struct objc_assoc_fake_class *)_objc_find_class_for_object(self);
	
	struct reference_list *list = &cl->list;
	
	objc_remove_associated_objects(self);
	
	/* 
	 * The lock names need to be unique, so we're actually allocating the name.
	 */
	objc_dealloc((void*)list->lock.name, M_FAKE_CLASS_TYPE);
	objc_rw_lock_destroy(&list->lock);
	free_dtable((dtable_t*)&cl->dtable);
	
	objc_dealloc(cl, M_FAKE_CLASS_TYPE);
}

/*
 * Looks for the fake class in the class hierarchy. If not found
 * and create == YES, allocates it and installs the isa pointer.
 */
static Class
_objc_class_for_object(id object, BOOL create)
{
	Class superclass = object->isa;
	Class cl = _objc_find_class_for_object(object);
	if (cl == Nil && create){
		volatile int *spin_lock = lock_for_pointer(object);
		lock_spinlock(spin_lock);
		
		cl = _objc_find_class_for_object(object);
		if (cl != Nil){
			/* Someone was faster. */
			return cl;
		}
		
		cl = objc_zero_alloc(sizeof(struct objc_assoc_fake_class),
				     M_FAKE_CLASS_TYPE);
		cl->isa = superclass->isa;
		cl->super_class = superclass;
		cl->dtable = uninstalled_dtable;
		cl->flags.fake = YES;
		cl->flags.resolved = YES;
		
		class_addMethod(cl,
				objc_cxx_destruct_selector,
				(IMP)_objc_associated_object_cxx_destruct);

		/*
		 * The lock names need to be unique, so we're actually allocating the 
		 * name. We have a common prefix, that is suffixed by the object's 
		 * class' name and a hex of the obj pointer.
		 */
		const char *name_prefix = "objc_assoc_fake_class_lock_";
		const char *class_name = object_getClassName(object);
		
		/* We need a format 0x1234 - 0x == 2 + sizeof(void*)*2 */
		unsigned int pointer_str_len = (sizeof(void*) * 2) + 2;
		char pointer_str[pointer_str_len + 1];
		objc_format_string(pointer_str, "%p", object);
		pointer_str[pointer_str_len] = '\0'; /* NULL termination */
		
		/* Now concat it */
		unsigned int name_prefix_len = objc_strlen(name_prefix);
		unsigned int class_name_len = objc_strlen(class_name);
		unsigned int lock_name_len = name_prefix_len + class_name_len
															+ pointer_str_len;
		char *lock_name = objc_alloc(lock_name_len, M_FAKE_CLASS_TYPE);
		objc_copy_memory(lock_name, name_prefix, name_prefix_len);
		objc_copy_memory(lock_name + name_prefix_len, class_name,
						 class_name_len);
		objc_copy_memory(lock_name + name_prefix_len + class_name_len,
						 pointer_str, pointer_str_len + 1); /* +1 for NULL-term */
		
		objc_debug_log("Created lock name: %s\n", lock_name);
		
		objc_rw_lock_init(&((struct objc_assoc_fake_class*)cl)->list.lock,
						  lock_name);
		
		object->isa = cl;
		unlock_spinlock(spin_lock);
		
		objc_debug_log("Created fake class (%s) for object %p\n",
			       class_getName(cl),
			       object);
	}
	return cl;
}

/*
 * Returns a reference to the objc_object_ref_list struct for
 * that particular object, or NULL if none exists and create == NULL.
 */
static struct reference_list *
_objc_ref_list_for_object(id object, BOOL create)
{
	if (object->isa->flags.meta){
		/*
		 * It's a class - install the associated object directly onto 
		 * the metaclass.
		 */
		Class cl = object->isa;
		void **extra_space;
		extra_space = objc_class_extra_with_identifier(cl,
					OBJC_ASSOCIATED_OBJECTS_IDENTIFIER);
		if (*extra_space == NULL && create){
			struct reference_list *list;
			list = objc_zero_alloc(sizeof(struct reference_list),
					       M_REFLIST_TYPE);
			
			objc_rw_lock_init(&list->lock,
					  "objc_reference_list_lock");
			
			*extra_space = list;
		}
		return *extra_space;
	}
	
	/*
	 * It's not a class, create a fake class and update the isa pointer
	 * (if create == YES)
	 */
	Class cl = _objc_class_for_object(object, create);
	if (cl != Nil){
		return &((struct objc_assoc_fake_class*)cl)->list;
	}
	return NULL;
}

/*
 * Returns YES when the policy is one of the atomic ones.
 */
static inline BOOL
_objc_is_policy_atomic(objc_AssociationPolicy policy)
{
	static objc_AssociationPolicy const OBJC_ASSOCIATION_ATOMIC = 0x300;
	return (policy & OBJC_ASSOCIATION_ATOMIC) == OBJC_ASSOCIATION_ATOMIC;
}

/*
 * Disposes of the object according to the policy. This pretty much means 
 * releasing the object unless the policy was assign.
 */
static inline void
_objc_dispose_of_object_according_to_policy(id object,
					    objc_AssociationPolicy policy)
{
	if (policy == OBJC_ASSOCIATION_WEAK_REF){
		void **address = (void**)object;
		if (address != NULL){
			*(void**)address = NULL;
		}
	}else if (policy != OBJC_ASSOCIATION_ASSIGN){
		objc_release(object);
	}
}

static inline struct reference *
_objc_find_key_reference_in_list(struct reference_list *list, void *key)
{
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

static inline struct reference *
_objc_find_free_reference_in_list(struct reference_list *list, BOOL create)
{
	while (list != NULL){
		for (int i = 0; i < REF_CNT; ++i){
			if (list->refs[i].key == NULL){
				return &list->refs[i];
			}
		}
		
		/*
		 * Haven't found any free ref field and we're at the end of the
		 * linked list - if creation is allowed, allocate.
		 */
		if (list->next == NULL && create){
			list->next = objc_zero_alloc(
					     sizeof(struct reference_list),
					     M_REFLIST_TYPE);
			return &list->next->refs[0];
		}
		
		list = list->next;
	}
	return NULL;
}

static void
_objc_remove_associative_list(struct reference_list *prev,
			      struct reference_list *list,
			      volatile int *lock,
			      BOOL free){
	if (list->next != NULL){
		_objc_remove_associative_list(list, list->next, lock, free);
	}
	
	/*
	 * Need to lock it for the pointer reassignement.
	 */
	if (prev != NULL){
		lock_spinlock(lock);
		prev->next = list->next;
		unlock_spinlock(lock);
	}
	
	for (int i = 0; i < REF_CNT; ++i){
		struct reference *ref = &list->refs[i];
		if (ref->key != NULL){
			_objc_dispose_of_object_according_to_policy(ref->value,
								ref->policy);
		}
		ref->key = NULL;
		ref->value = NULL;
		ref->policy = 0;
	}
	
	if (free){
		objc_dealloc(list, M_REFLIST_TYPE);
	}
}

static inline void
_objc_remove_associative_lists_for_object(id object)
{
	volatile int *spin_lock = lock_for_pointer(object);
	if (object->isa->flags.meta){
		void **extra_space;
		extra_space = objc_class_extra_with_identifier(object->isa,
					OBJC_ASSOCIATED_OBJECTS_IDENTIFIER);
		if (*extra_space == NULL){
			return;
		}
		
		struct reference_list *list = *extra_space;
		if (list->next != NULL){
			_objc_remove_associative_list(list,
						      list->next,
						      spin_lock,
						      NO);
		}
		objc_rw_lock_destroy(&list->lock);
		_objc_remove_associative_list(NULL,
					      list,
					      spin_lock,
					      YES);
	}else{
		struct objc_assoc_fake_class *cl;
		cl = (struct objc_assoc_fake_class*)
				_objc_class_for_object(object, NO);
		if (cl == NULL){
			return;
		}
		
		if (cl->list.next != NULL){
			_objc_remove_associative_list(&cl->list, cl->list.next,
						      spin_lock, YES);
		}
		objc_rw_lock_destroy(&cl->list.lock);
		_objc_remove_associative_list(NULL, &cl->list,
					      spin_lock, NO);
	}
}


#pragma mark -
#pragma mark Public Functions

id
objc_get_associated_object(id object, void *key)
{
	if (object == nil || key == NULL){
		return nil;
	}
	
	if (objc_object_is_small_object(object)){
		return nil;
	}
	
	id result = nil;
	
	struct reference_list *list = _objc_ref_list_for_object(object, NO);
	if (list == NULL){
		return nil;
	}
	
	objc_rw_lock_rlock(&list->lock);
	struct reference *ref = _objc_find_key_reference_in_list(list, key);
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
				/* This is atomic, need to lock the WLOCK */
				objc_rw_lock_wlock(&list->lock);
				result = objc_copy(ref->value);
				objc_rw_lock_unlock(&list->lock);
				break;
			case OBJC_ASSOCIATION_RETAIN:
				/* This is atomic, need to lock the WLOCK */
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

void
objc_set_associated_object(id object, void *key, id value,
			   objc_AssociationPolicy policy)
{
	if (object == nil || key == NULL){
		return;
	}
	
	if (objc_object_is_small_object(object)){
		return;
	}
	
	struct reference_list *list = _objc_ref_list_for_object(object, YES);
	
	
	/* WLOCK because we may be actually modifying the linked list */
	objc_rw_lock_wlock(&list->lock);
	struct reference *ref = _objc_find_key_reference_in_list(list, key);
	if (ref == NULL){
		/* No reference, perhaps need to allocate some */
		ref = _objc_find_free_reference_in_list(list, YES);
		objc_assert(ref != NULL,
			    "_objc_reference_create_for_object hasn't"
			    " created reference list for object %p!\n", object);
	}
	
	
	/*
	 * See whether either policy is atomic and if not, we can unlock the
	 * lock. Otherwise, keep it locked and unlock it after all the 
	 * modification of ref.
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
	
	
	objc_AssociationPolicy old_policy = ref->policy;
	id old_value = ref->value;
	
	ref->key = key;
	ref->policy = policy;
	ref->value = value;
	
	_objc_dispose_of_object_according_to_policy(old_value, old_policy);
	
	
	/*
	 * If either of the policies was atomic, the lock is still locked, need
	 * to unlock it.
	 */
	if (either_policy_atomic){
		objc_rw_lock_unlock(&list->lock);
	}
	
}

int
objc_sync_enter(id obj)
{
	if (obj == nil || objc_object_is_small_object(obj)){
		return 0;
	}
	struct reference_list *list = _objc_ref_list_for_object(obj, YES);
	return objc_rw_lock_wlock(&list->lock);
}

int
objc_sync_exit(id obj)
{
	if (obj == nil || objc_object_is_small_object(obj)){
		return 0;
	}
	struct reference_list *list = _objc_ref_list_for_object(obj, NO);
	if (list == NULL){
		return 0;
	}
	
	return objc_rw_lock_unlock(&list->lock);
}

void
objc_remove_associated_objects(id object)
{
	if (object == nil){
		return;
	}
	
	if (objc_object_is_small_object(object)){
		return;
	}
	
	_objc_remove_associative_lists_for_object(object);
}

void
objc_remove_associated_weak_refs(id object)
{
	
	if (objc_object_is_small_object(object)){
		/* Small objects do not have associated objects */
		return;
	}
	
	/*
	 * This function is quite similar to the objc_remove_associated_objects,
	 * but doesn't actually free any objects, or any of the
	 * objc_object_ref_list structures.
	 */
	
	struct reference_list *list = _objc_ref_list_for_object(object, NO);
	if (list == NULL){
		/* Nothing to do */
		return;
	}
	
	/*
	 * Go through the lists and remove all weak refs.
	 */
	
	objc_rw_lock *lock = &list->lock;
	objc_rw_lock_wlock(lock);
	
	while (list != NULL) {
		for (int i = 0; i < REF_CNT; ++i){
			struct reference *ref = &list->refs[i];
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
	
	objc_rw_lock_unlock(lock);
}


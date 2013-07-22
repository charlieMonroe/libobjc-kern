
#include "class_extra.h"

#define spinlock_do_not_allocate_page 1
#include "spinlock.h"

/*
 * It is expected to happen only once per class per class extra (currently
 * only used by associated objects) and in current implementation only when
 * associating objects directly with a class, which is not often.
 *
 * Therefore a spinlock is used instead of using a RW lock or mutex, since
 * the contention is expected to be minimal if any.
 */
volatile int class_extra_spinlock;

struct objc_class_extra {
	struct objc_class_extra		*next;
	void				*data;
	unsigned int			identifier;
};

/*
 * Finds an extra space on the class cl that has 'identifier'.
 * Returns NULL if not found.
 */
static inline struct objc_class_extra *
_objc_class_extra_find_no_lock(Class cl, unsigned int identifier)
{
	struct objc_class_extra *extra = NULL;
	if (cl->extra_space != NULL){
		extra = (struct objc_class_extra*)cl->extra_space;
		while (extra != NULL) {
			if (extra->identifier == identifier){
				break;
			}
			extra = extra->next;
		}
	}
	
	return extra;
}

/*
 * Calls the above method, surrounded by RLOCK.
 */
static inline struct objc_class_extra *
_objc_class_extra_find(Class cl, unsigned int identifier)
{
	
	lock_spinlock(&class_extra_spinlock);
	struct objc_class_extra *extra;
	extra = _objc_class_extra_find_no_lock(cl, identifier);
	unlock_spinlock(&class_extra_spinlock);
	
	return extra;
}

/*
 * Locks the WLOCK, and if the extra with the identifier cannot be
 * found, creates one and installs it onto class.
 */
static inline struct objc_class_extra *
_objc_class_extra_create(Class cl, unsigned int identifier)
{
	lock_spinlock(&class_extra_spinlock);
	
	/* See if anyone hasn't added the extra in the meanwhile */
	struct objc_class_extra *extra;
	extra = _objc_class_extra_find_no_lock(cl, identifier);
	if (extra == NULL){
		// Still NULL, need to allocate it
		extra = objc_alloc(sizeof(struct objc_class_extra));
		extra->next = NULL;
		extra->identifier = identifier;
		extra->data = NULL;
		
		// Insert it
		if (cl->extra_space == NULL){
			cl->extra_space = extra;
		}else{
			// Need to insert it to the end of the chain
			struct objc_class_extra *e = cl->extra_space;
			while (e->next != NULL){
				e = e->next;
			}
			e->next = extra;
		}
	}
	
	unlock_spinlock(&class_extra_spinlock);
	
	return extra;
}

/*
 * See header for docs.
 */
PRIVATE void **
objc_class_extra_with_identifier(Class cl, unsigned int identifier)
{
	struct objc_class_extra *extra = _objc_class_extra_find(cl, identifier);
	if (extra == NULL){
		extra = _objc_class_extra_create(cl, identifier);
	}
	return &extra->data;
}


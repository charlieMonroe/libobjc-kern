#include "class_extra.h"

struct objc_class_extra {
	struct objc_class_extra		*next;
	void				*data;
	unsigned int			identifier;
};


/*
 * RW lock that guards all access to Class->extra_space.
 */
// TODO use spinlock
static objc_rw_lock objc_class_extra_lock;

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
	objc_rw_lock_rlock(&objc_class_extra_lock);
	struct objc_class_extra *extra;
	extra = _objc_class_extra_find_no_lock(cl, identifier);
	objc_rw_lock_unlock(&objc_class_extra_lock);
	
	return extra;
}

/*
 * Locks the WLOCK, and if the extra with the identifier cannot be
 * found, creates one and installs it onto class.
 */
static inline struct objc_class_extra *
_objc_class_extra_create(Class cl, unsigned int identifier)
{
	objc_rw_lock_wlock(&objc_class_extra_lock);
	
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
	
	objc_rw_lock_unlock(&objc_class_extra_lock);
	
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


PRIVATE void objc_class_extra_init(void){
	objc_rw_lock_init(&objc_class_extra_lock);
}

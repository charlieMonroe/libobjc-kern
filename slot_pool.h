#define POOL_NAME slot
#define POOL_TYPE struct objc_slot
#define POOL_MALLOC_TYPE M_SLOT_POOL_TYPE
#include "pool.h"
#include "utils.h"

/*
 * Allocates a new slot and initialises it for this method.
 */
static inline struct objc_slot *
objc_slot_create_for_method_in_class(Method method, Class class)
{
	
	struct objc_slot *slot = slot_pool_alloc();
	slot->owner = class;
	slot->types = method->selector_name +
		      objc_strlen(method->selector_name) + 1;
	slot->selector = method->selector;
	slot->implementation = method->implementation;
	slot->version = 1;
	
	return slot;
}

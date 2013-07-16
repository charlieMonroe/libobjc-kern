
#ifndef OBJC_ARC_H
#define OBJC_ARC_H

#include "types.h"

// Autorelease-pool related stuff
void *objc_autorelease_pool_push(void);
void objc_autorelease_pool_pop(void *pool);

// ARR
id objc_autorelease(id obj);
id objc_retain(id obj);
void objc_release(id obj);
id objc_retain_autorelease(id obj);
id objc_store_strong(id *addr, id value);
id objc_load_weak(id *object);
id objc_store_weak(id *addr, id value);


void objc_delete_weak_refs(id obj);
id objc_load_weak_retained(id *addr);
id objc_load_weak(id *obj);
void objc_copy_weak(id *dest, id *src);
void objc_move_weak(id *dest, id *src);
void objc_destroy_weak(id *obj);
id objc_init_weak(id *object, id value);

void objc_delete_weak_refs(id obj);


/**
 * Simply send the -copy message.
 */
id objc_copy(id obj);

#endif

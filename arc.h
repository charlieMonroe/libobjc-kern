
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


void objc_delete_weak_refs(id obj);

#endif


#ifndef OBJC_ARC_H
#define OBJC_ARC_H

#include "types.h"

/* Autorelease-pool related stuff */
void	objc_autoreleasePoolPop(void *pool);
void	*objc_autoreleasePoolPush(void);

/* ARR */
id	objc_autorelease(id obj);
void	objc_copyWeak(id *dest, id *src);
void	objc_delete_weak_refs(id obj);
void	objc_destroyWeak(id *obj);
id	objc_initWeak(id *object, id value);
id 	objc_loadWeak(id *obj);
id	objc_loadWeakRetained(id *addr);
void	objc_moveWeak(id *dest, id *src);
void	objc_release(id obj);
id	objc_retain(id obj);
id	objc_retainAutorelease(id obj);
id	objc_storeStrong(id *addr, id value);
id	objc_storeWeak(id *addr, id value);

/*
 * Simply sends the -copy message.
 */
id	objc_copy(id obj);

#endif

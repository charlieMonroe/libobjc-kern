
#ifndef ASSOCIATED_OBJECTS_H_
#define ASSOCIATED_OBJECTS_H_

#include "../os.h"

/**
 * Registers the AO extension with the run-time.
 */
extern void objc_associated_object_register_extension(void);

/**
 * Returns an object associated with obj under key.
 */
extern id objc_object_get_associated_object(id obj, void *key);

/**
 * Sets an object value associated with obj under key.
 */
extern void objc_object_set_associated_object(id obj, void *key, id value);


#endif /* ASSOCIATED_OBJECTS_H_ */

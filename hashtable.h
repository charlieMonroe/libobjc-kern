
#ifndef OBJC_HASHTABLE_H
#define OBJC_HASHTABLE_H

/**
 * This is a template for a generic hash table that
 * is used throughout the run-time for hashing
 * mostly objects based on their name.
 *
 * Before including this header, you need to define
 * several macros that serve as options for the hash
 * table:
 *
 * OBJC_HASHTABLE_NAME - name of the static variable.
 * OBJC_HASHTABLE_PREFIX - prefix of the hash table ops.
 * OBJC_HASHTABLE_VALUE_TYPE - type of the values stored 
 * in the hash table.
 *
 */

#include "os.h"

#ifndef OBJC_HASHTABLE_NAME
#error You need to define OBJC_HASHTABLE_NAME first.
#endif

#define REALLY_PREFIX_SUFFIX(x, y) x ## y
#define PREFIX_SUFFIX(x, y) REALLY_PREFIX_SUFFIX(x, y)
#define PREFIX(x) PREFIX_SUFFIX(MAP_TABLE_NAME, x)

typedef struct {
	
} PREFIX(_hashtable_bucket_t);

#define OBJC_HASHTABLE_BUCKET_T PREFIX(_hashtable_bucket_t)

/**
 * The actual hash table.
 */
typedef struct objc_hashtable {
	objc_lock *lock;
	unsigned int table_size;
	unsigned int used;
	
	
	
	OBJC_HASHTABLE_BUCKET_T *old;
	OBJC_HASHTABLE_BUCKET_T *table;
} objc_hashtable_t;



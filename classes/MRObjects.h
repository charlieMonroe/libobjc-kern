
#ifndef _MRObject_H_
#define _MRObject_H_

#include "../types.h"

/*
 * Structure of a MRObject instance.
 */
typedef struct {
	Class isa;
	int retainCount;
} MRObject_instance_t;

/*
 * Structure of a __MRConstrString instance.
 */
typedef struct {
	Class isa;
	int retainCount;
	const char *cString;
} __MRConstString_instance_t;

/* 
 * Exported class prototypes.
 *
 * Note that the same structures are then
 * casted to classes.
 */
extern struct objc_class MRObject_class;
extern struct objc_class __MRConstString_class;

/*
 * A macro for creating a constant string instance.
 */
#define OBJC_STRING(VAR_NAME, STR) static __MRConstString_instance_t VAR_NAME##_stat_str = { (Class)(&__MRConstString_class), 1, STR };\
							     id VAR_NAME = (id)(&VAR_NAME##_stat_str);


#endif /* _MRObject_H_ */

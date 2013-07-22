
#ifndef OBJC_TYPES_H
#define OBJC_TYPES_H

#include "os.h"
#include "kernobjc/types.h"

/*
 * Actual declarations of the structures follow.
 */
struct objc_selector {
	/* 
	 * Name of the selector, followed by \0,
	 * followed by the types, followed by \0.
	 *
	 * For example: "init\0@@:\0"
	 */
	const char	*name;
	
	/*
	 * On registering, the selUID is populated and is
	 * the pointer into the selector table.
	 */
	uint16_t	sel_uid;
};

struct objc_method {
	IMP		implementation;
	
	/*
	 * Need to include name and type since the 16-bit selector sel_uid
	 * is unknown at compile time and is populated at load time.
	 */
	const char	*selector_name;
	const char	*selector_types;
	
	SEL		selector;
	
	unsigned int	version;
};

/*
 * Declaration of an Ivar.
 */
struct objc_ivar {
	const char	*name;
	const char	*type;
	size_t		size;
	uintptr_t	offset; /* Filled by runtime. */
	uint8_t		align;
};

struct objc_property {
	const char	*name;
	const char	*getter_name;
	const char	*setter_name;
	
	const char	*getter_types;
	const char	*setter_types;
	
	char		attributes;
	char		attributes2;
};


struct objc_category {
	const char	*category_name;
	const char	*class_name;
	
	objc_method_list *instance_methods;
	objc_method_list *class_methods;
	objc_protocol_list *protocols;
};

typedef struct {
	BOOL		meta : 1;
	BOOL		resolved : 1;
	BOOL		initialized : 1; /* +initialized called */
	BOOL		user_created : 1;
	
	/* Implements its own -retain, -release, or -autorelease */
	BOOL		has_custom_arr : 1;
	
	/*
	 * The class is a fake and doesn't have any of the fields beyond 
	 * the flags.
	 */
	BOOL		fake : 1;
} objc_class_flags;

#define OBJC_CLASS_COMMON_FIELDS					\
	Class			isa; /* Points to meta class. */	\
	Class			super_class;				\
	void			*dtable; /* Disptach table. */		\
	objc_class_flags	flags; /* Flags. */			

/* Actual structure of Class. */
struct objc_class {
	OBJC_CLASS_COMMON_FIELDS;
	
	const char		*name;
	
	/*
	 * On class registration, the run-time calculates
	 * offsets of all ivars, allocs an array here and 
	 * populates it with ivar offsets.
	 */
	size_t			*ivar_offsets;
	
	/*
	 * WARNING: All of the lists are lazily created -> may be NULL!
	 */
	objc_method_list	*methods;
	objc_ivar_list		*ivars;
	objc_category_list	*categories;
	objc_protocol_list	*protocols;
	objc_property_list	*properties;
		
	Class			subclass_list;
	Class			sibling_list;
	
	Class			unresolved_class_previous;
	Class			unresolved_class_next;
	
	/*
	 * Extra space for the run-time to use. Currently used for objects
	 * associated with classes.
	 */
	void			*extra_space;
	
	size_t			instance_size;
	int			version; /* Right now 0. */
};


struct objc_slot {
	Class owner;
	Class cachedFor;
	IMP implementation;
	const char *types;
	unsigned int version;
	SEL selector;
};

#include "list_types.h"
#include "loader.h"

#endif /* !OBJC_TYPES_H */

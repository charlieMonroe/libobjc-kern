/*
 * This file contains basic types, like BOOL and NULL.
 */

#ifndef OBJC_TYPES_H_
#define OBJC_TYPES_H_

#include "os.h"

/* A boolean type. */
typedef signed char BOOL;
#define YES ((BOOL)1)
#define NO ((BOOL)0)

/* NULL definition. */
#ifndef NULL
	#define NULL ((void*)0)
#endif

/**
 * Definitions of nil and Nil.
 * nil is used for objects, Nil for classes. It doesn't really matter,
 * but all traditional run-times use this as "type checking".
 */
#define nil ((id)0)
#define Nil ((Class)0)

 
/**
 * Forward declarations.
 */
typedef struct objc_class *Class;
typedef struct objc_selector *Selector;
typedef struct objc_ivar *Ivar;
typedef struct objc_category *Category;
typedef struct objc_method *Method;
typedef struct objc_property *Property;
typedef struct objc_protocol Protocol;
typedef struct objc_slot *Slot;
typedef struct objc_object *id;

/* A definition of a SEL. */
typedef uint16_t SEL;

/* A definition of a method implementation function pointer. */
typedef id(*IMP)(id target, SEL _cmd, ...);


/**
 * Used generally only in protocols to describe a method.
 */
struct objc_method_description {
	/**
	 * The types of this method.
	 */
	const char *types;
	
	/**
	 * The name of this method.
	 */
	SEL   selector;
};

/**
 * Actual declarations of the structures follow.
 */
struct objc_selector {
	/* 
	 * Name of the selector, followed by \0,
	 * followed by the types, followed by \0.
	 *
	 * For example: "init\0@@:\0"
	 */
	const char *name;
	
	/*
	 * On registering, the selUID is populated and is
	 * the pointer into the selector table.
	 */
	uint16_t sel_uid;
};

struct objc_method {
	IMP implementation;
	
	// Need to include name and type
	// since the 16-bit selector sel_uid
	// is unknown at compile time and
	// is populated at load time.
	const char *selector_name;
	const char *selector_types;
	
	SEL selector;
	
	unsigned int version;
};

/**
 * Definition of id - a pointer to an object - a struct, where the first field
 * is so-called isa, pointer to the class the object is an instance of.
 */
struct objc_object {
	Class isa;
};

/**
 * Definition of super. As the super calls may
 * be chained, this is quite necessary.
 */
typedef struct {
	id receiver;
	Class class;
} objc_super;

/**
 * Declaration of an Ivar.
 */
struct objc_ivar {
	const char *name;
	const char *type;
	unsigned int size;
	unsigned int offset;
	unsigned int align;
};

struct objc_property {
	const char *name;
	const char *getter_name;
	const char *setter_name;
	
	const char *getter_types;
	const char *setter_types;
	
	// TODO - merge attributes into
	// on unsigned int? Would require
	// to redifine enums in property.h
	char attributes;
	char attributes2;
};

typedef struct {
	const char *name;
	const char *value;
	size_t length;
	
	// TODO
} objc_property_attribute_t;


/**
 * Need to make forward typedefs since they are used
 * in structures which are needed in the actual defs.
 */
typedef struct objc_protocol_list_struct objc_protocol_list;
typedef struct objc_method_list_struct objc_method_list;
typedef struct objc_method_description_list_struct objc_method_description_list;
typedef struct objc_property_list_struct objc_property_list;
typedef struct objc_ivar_list_struct objc_ivar_list;
typedef struct objc_category_list_struct objc_category_list;


struct objc_protocol {
	Class isa;
	const char *name;
	
	objc_protocol_list *protocols; // other protocols
	objc_method_description_list *instance_methods;
	objc_method_description_list *class_methods;
	
	objc_method_description_list *optional_instance_methods;
	objc_method_description_list *optional_class_methods;
	
	objc_property_list *properties;
	objc_property_list *optional_properties;
};

struct objc_category {
	const char *category_name;
	const char *class_name;
	
	objc_method_list *instance_methods;
	objc_method_list *class_methods;
	objc_protocol_list *protocols;
};

typedef struct {
	BOOL meta : 1;
	BOOL resolved : 1;
	BOOL initialized : 1; // +initialized called
	BOOL user_created : 1;
	BOOL has_custom_arr : 1; // Implements -retain, -release, or -autorelease
	BOOL fake : 1; // The class is a fake and doesn't have any of the fields beyond the flags
} objc_class_flags;

/* Actual structure of Class. */
struct objc_class {
	Class isa; /* Points to meta class */
	Class super_class;
	void *dtable;
	objc_class_flags flags;
	
	const char *name;
	
	/**
	 * On class registration, the run-time calculates
	 * offsets of all ivars, allocs an array here and 
	 * populates it with ivar offsets.
	 */
	unsigned int *ivar_offsets;
	
	/*
	 * Both class and instance methods and ivars are actually
	 * arrays of arrays - each item of this objc_array is a pointer
	 * to a C array of methods/ivars.
	 *
	 * WARNING: All of them are lazily created -> may be NULL!
	 */
	objc_method_list *methods;
	objc_ivar_list *ivars;
	objc_category_list *categories;
	objc_protocol_list *protocols;
	objc_property_list *properties;
		
	Class subclass_list;
	Class sibling_list;
	
	Class unresolved_class_previous;
	Class unresolved_class_next;
	
	void *extra_space;
	
	unsigned int instance_size;
	unsigned int version; /** Right now 0. */
};


struct objc_slot {
	Class owner, cachedFor;
	const char *types;
	SEL selector;
	IMP method;
	unsigned int version;
};

#include "list_types.h"
#include "loader.h"

#endif /* OBJC_TYPES_H_ */

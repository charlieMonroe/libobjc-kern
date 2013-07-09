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
typedef struct objc_protocol *Protocol;
typedef struct objc_object *id;

/* A definition of a SEL. */
typedef unsigned short SEL;

/* A definition of a method implementation function pointer. */
typedef id(*IMP)(id target, SEL _cmd, ...);


/**
 * Actual declarations of the structures follow.
 * TODO: decide whether to make them opaque,
 * declared in a private header/source files.
 */
struct objc_selector {
	const char *name; /* Name of the selector. */
	const char *types; /* Types of the selector. */
	
	/*
	 * On registering, the selUID is populated and is
	 * the pointer into the selector table.
	 */
	SEL selUID;
};

// TODO - include types? They're already in the selector...
struct objc_method {
	IMP implementation;
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
	
	// TODO flags
	unsigned int flags;
	
};


#include "list_types.h"


struct objc_protocol {
	Class isa;
	const char *name;
	
	objc_protocol_list *protocols; // other protocols
	objc_method_list *instance_methods;
	objc_method_list *class_methods;
	
	objc_method_list *optional_instance_methods;
	objc_method_list *optional_class_methods;
	
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


/* Actual structure of Class. */
struct objc_class {
	Class isa; /* Points to meta class */
	Class super_class;
	char *name;
	
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
	
	// TODO
	// Do we want subclasses and siblings like the GNUStep?
	
	/* Cache/Dispatch table */
	void *cache;
	
	unsigned int instance_size;
	unsigned int version; /** Right now 0. */
	
	struct {
		BOOL is_meta : 1;
		BOOL in_construction : 1;
	} flags;
};

#endif /* OBJC_TYPES_H_ */

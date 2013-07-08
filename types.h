/*
 * This file contains basic types, like BOOL and OBJC_NULL.
 */

#ifndef OBJC_TYPES_H_
#define OBJC_TYPES_H_

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

 
/********** Class ***********/
/**
 * Declaration of a Class.
 */

typedef struct objc_class *Class;

/* A definition of a SEL. */
typedef unsigned short SEL;

typedef struct objc_selector {
	const char *name; /* Name of the selector. */
	const char *types; /* Types of the selector. */
	
	/* 
	 * On registering, the selUID is populated and is
	 * the pointer into the selector table.
	 */
	SEL selUID;
} objc_selector_t;

/**
 * Definition of id - a pointer to an object - a struct, where the first field
 * is so-called isa, pointer to the class the object is an instance of.
 */
typedef struct objc_object {
	Class isa;
} *id;

/**
 * Definition of super. As the super calls may
 * be chained, this is quite necessary.
 */
typedef struct {
	id receiver;
	Class class;
} objc_super;

/* A definition of a method implementation function pointer. */
typedef id(*IMP)(id target, SEL _cmd, ...);

/**
 * Declaration of a Method.
 */
typedef struct objc_method {
	IMP implementation;
	
	objc_selector_t *selector;
	
	// Do we need types here, since they are already in the selector,
	// selector is typed and we do not allow two selectors with
	// different types?
	const char *types;
	
	unsigned int version;
} *Method;


/**
 * Declaration of an Ivar.
 */
typedef struct objc_ivar {
	const char *name;
	const char *type;
	unsigned int size;
	unsigned int offset;
	unsigned int align;
} *Ivar;



struct objc_category {
	const char *category_name;
	const char *class_name;
	
	objc_array instance_methods;
	objc_array class_methods;
	objc_array protocols;
} *Category;


struct objc_property {
	const char *name;
	const char *getter_name;
	const char *setter_name;
	
	const char *getter_types;
	const char *setter_types;
	
	// TODO flags
	unsigned int flags;
	
} *Property;

struct objc_protocol {
	Class isa;
	const char *name;
	
	objc_array protocols; // other protocols
	objc_array instance_methods;
	objc_array class_methods;
	
	objc_array optional_instance_methods;
	objc_array optional_class_methods;
	
	objc_array properties;
	objc_array optional_properties;
}


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
	objc_array methods;
	objc_array ivars;
	objc_array categories;
	objc_array protocols;
	objc_array properties;
	
	// TODO
	// Do we want subclasses and siblings like the GNUStep?
	
	/* Cache/Dispatch table */
	objc_cache cache;
	
	unsigned int instance_size;
	unsigned int version; /** Right now 0. */
	
	struct {
		BOOL is_meta : 1;
		BOOL in_construction : 1;
	} flags;
};


#endif /* OBJC_TYPES_H_ */

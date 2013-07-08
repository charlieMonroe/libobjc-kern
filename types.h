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
 
/********** Class ***********/
/**
 * Declaration of a Class.
 */

typedef struct objc_class *Class;

/* A definition of a SEL. */
typedef unsigned short SEL;

typedef struct objc_selector {
	const char *name;
	const char *types;
} objc_selector_t;

/**
 * Definition of id - a pointer to an object - a struct, where the first field
 * is so-called isa, in GCC run-time called 'class_pointer'. I've chosen to keep
 * the 'isa' name.
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
	SEL selector;
	const char *types;
	IMP implementation;
	unsigned int version;
} *Method;


/**
 * The same as objc_method, but char * instead
 * of SEL.
 */
struct objc_method_prototype {
	const char *selector_name;
	const char *types;
	IMP implementation;
	unsigned int version;
};

/**
 * Declaration of an Ivar.
 */
typedef struct objc_ivar {
	const char *name;
	const char *type;
	unsigned int size;
	unsigned int offset;
} *Ivar;

/**
 * Definitions of nil and Nil.
 * nil is used for objects, Nil for classes. It doesn't really matter,
 * but all traditional run-times use this as "type checking".
 */
#define nil ((id)0)
#define Nil ((Class)0)
 
/**
 * A definition for a class holder - a data structure that is responsible for
 * keeping a list of classes registered with the run-time, looking them up, etc.
 * Priority of this structure should be lookup speed.
 */
typedef void *objc_class_holder;

/**
 * A definition for a selector holder - a data structure that is responsible for
 * keeping a list of selectors registered with the run-time, looking them up, etc.
 * Priority of this structure should be both lookup and insertion speed.
 */
typedef void *objc_selector_holder;

/**
 * A definition for a cache - a data structure that is responsible for
 * keeping a list of IMPs to be found by SEL.
 * Priority of this structure should be both lookup and insertion speed.
 */
typedef void *objc_cache;

/**
 * A definition for a dynamically growing array structure. The easiest
 * implementation is to create a structure which includes a counter of objects,
 * size of the array, and a dynamically allocated C-array of inserted pointers.
 * It is used to keep arrays of methods, protocols, etc. on a class.
 * This run-time provides a default implementation of such an array.
 */
typedef void *objc_array;

/**
 * An enumerator structure returned by the array.
 */
typedef struct _objc_array_enumerator {
	struct _objc_array_enumerator *next;
	void *item;
} *objc_array_enumerator;

/* A definition for a read/write lock. */
typedef void *objc_rw_lock;


/* Actual structure of Class. */
struct objc_class {
	Class isa; /* Points to self - this way the lookup mechanism detects class method calls */
	Class super_class;
	char *name;
	
	/*
	 * Both class and instance methods and ivars are actually
	 * arrays of arrays - each item of this objc_array is a pointer
	 * to a C array of methods/ivars.
	 *
	 * WARNING: All of them are lazily created -> may be NULL!
	 */
	objc_array class_methods;
	objc_array instance_methods;
	objc_array ivars;
	
	/* Cache */
	objc_cache class_cache;
	objc_cache instance_cache;
	
	unsigned int instance_size; /* Doesn't include class extensions */
	unsigned int version; /** Right now 0. */
	struct {
		BOOL in_construction : 1;
	} flags;
	
	void *extra_space;
};

/** Class prototype. */
struct objc_class_prototype {
	Class isa; /* Must be NULL! */
	const char *super_class_name;
	const char *name;
	
	/** All must be NULL-terminated. */
	struct objc_method_prototype **class_methods;
	struct objc_method_prototype **instance_methods;
	Ivar *ivars;
	
	/* Cache - all pointers must be NULL */
	objc_cache class_cache;
	objc_cache instance_cache;
	
	unsigned int instance_size; /* Will be filled */
	unsigned int version; /** Right now 0. */
	struct {
		BOOL in_construction : 1; /* Must be YES */
	} flags;
	
	void *extra_space; /* Must be NULL */
};

#endif /* OBJC_TYPES_H_ */

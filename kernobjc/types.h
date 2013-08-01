
#ifndef LIBKERNOBJC_TYPES_H
#define LIBKERNOBJC_TYPES_H

/* A boolean type. */
typedef signed char BOOL;

#define YES ((BOOL)1)
#define NO ((BOOL)0)

#define null_selector ((SEL)0)

/*
 * Definitions of nil and Nil.
 * nil is used for objects, Nil for classes. It doesn't really matter,
 * but all traditional run-times use this as "type checking".
 */
#define nil ((id)0)
#define Nil ((Class)0)

typedef struct objc_class *Class;
typedef struct objc_ivar *Ivar;
typedef struct objc_category *Category;
typedef struct objc_method *Method;
#ifdef __OBJC__
@class Protocol;
#else
typedef struct objc_protocol Protocol;
#endif
typedef struct objc_object *id;

typedef struct objc_property *Property;
typedef Property objc_property_t;

/*
 * Need to make forward typedefs since they are used
 * in structures which are needed in the actual defs.
 */
typedef struct objc_protocol_list_struct objc_protocol_list;
typedef struct objc_method_list_struct objc_method_list;
typedef struct objc_method_description_list_struct objc_method_description_list;
typedef struct objc_property_list_struct objc_property_list;
typedef struct objc_ivar_list_struct objc_ivar_list;
typedef struct objc_category_list_struct objc_category_list;

/* A definition of a SEL. */
typedef uint16_t SEL;

/* A definition of a method implementation function pointer. */
typedef id(*IMP)(id target, SEL _cmd, ...);


/*
 * Used generally only in protocols to describe a method.
 */
struct objc_method_description {
	/*
	 * The selector name of this method.
	 */
	const char *selector_name;
	
	/*
	 * The types of this method.
	 */
	const char *selector_types;
	
	/*
	 * The name of this method.
	 */
	SEL   selector;
};

/*
 * Definition of id - a pointer to an object - a struct, where the first field
 * is so-called isa, pointer to the class the object is an instance of.
 */
struct objc_object {
	Class isa;
};

/*
 * Definition of super. As the super calls may
 * be chained, this is quite necessary.
 */
struct objc_super {
	id receiver;
	Class class;
};

typedef struct {
	const char *name;
	const char *value;
} objc_property_attribute_t;


struct objc_protocol {
	Class isa;
	const char *name;
	
	objc_protocol_list *protocols; /* other protocols */
	objc_method_description_list *instance_methods;
	objc_method_description_list *class_methods;
	
	objc_method_description_list *optional_instance_methods;
	objc_method_description_list *optional_class_methods;
	
	objc_property_list *properties;
	objc_property_list *optional_properties;
};


/*
 * Valid values for objc_AssociationPolicy.  This is really a bitfield, but
 * only specific combinations of flags are permitted.
 */
enum{
	/*
	 * Perform straight assignment, no message sends.
	 */
	OBJC_ASSOCIATION_ASSIGN = 0,
	
	/*
	 * Retain the associated object.
	 */
	OBJC_ASSOCIATION_RETAIN_NONATOMIC = 1,
	
	/*
	 * Copy the associated object, by sending it a -copy message.
	 */
	OBJC_ASSOCIATION_COPY_NONATOMIC = 3,
	
	/*
	 * Atomic retain.
	 */
	OBJC_ASSOCIATION_RETAIN = 0x301,
	
	/*
	 * Atomic copy.
	 */
	OBJC_ASSOCIATION_COPY = 0x303,
	
	/*
	 * A special association for storing weak refs. The weak refs are zeroed on
	 * either objc_remove_associated_objects or objc_remove_associated_weak_refs.
	 */
	OBJC_ASSOCIATION_WEAK_REF = 0x401
};

/*
 * Association policy, used when setting associated objects.
 */
typedef uintptr_t objc_AssociationPolicy;

#endif

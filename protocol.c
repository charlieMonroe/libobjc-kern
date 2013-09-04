#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "utils.h"
#include "property.h"
#include "selector.h"
#include "runtime.h"
#include "class.h"
#include "init.h"
#include "private.h"
#include "kernobjc/runtime.h"

static inline BOOL
_objc_protocols_are_equal(const char *name, Protocol *p)
{
	return objc_strings_equal(name, p->name);
}

static inline int
_objc_hash_protocol(Protocol *p)
{
	return objc_hash_string(p->name);
}

#define MAP_TABLE_NAME objc_protocol
#define MAP_TABLE_COMPARE_FUNCTION _objc_protocols_are_equal
#define MAP_TABLE_VALUE_TYPE Protocol *
#define MAP_TABLE_HASH_KEY objc_hash_string
#define MAP_TABLE_HASH_VALUE _objc_hash_protocol
#define MAP_MALLOC_TYPE M_PROTOCOL_MAP_TYPE
#include "hashtable.h"

#define BUFFER_TYPE objc_protocol_list
#include "buffer.h"

static objc_protocol_table *objc_protocols;
static Class objc_protocol_class;

static inline objc_method_description_list **
_objc_protocol_get_method_list_ptr(Protocol *p, BOOL required, BOOL instance)
{
	if (p == NULL){
		return NULL;
	}
	return (required ?
			(instance ? &p->instance_methods : &p->class_methods) :
			(instance ? &p->optional_instance_methods
			 : &p->optional_class_methods)
			);
}

static inline objc_property_list **
_objc_protocol_get_property_list_ptr(Protocol *p, BOOL required, BOOL instance)
{
	if (p == NULL || !instance){
		return NULL;
	}
	return (required ? &p->properties : &p->optional_properties);
}


static int
_objc_protocol_is_empty(Protocol *aProto)
{
	int isEmpty =
	((aProto->instance_methods == NULL) ||
	 (aProto->instance_methods->size == 0)) &&
	((aProto->class_methods == NULL) ||
	 (aProto->class_methods->size == 0)) &&
	((aProto->protocols == NULL) ||
	 (aProto->protocols->size == 0));
	
	isEmpty &= (aProto->optional_instance_methods->size == 0);
	isEmpty &= (aProto->optional_class_methods->size == 0);
	isEmpty &= (aProto->properties == 0) || (aProto->properties->size == 0);
	isEmpty &= (aProto->optional_properties == 0)
	|| (aProto->optional_properties->size == 0);
	return isEmpty;
}

static void
_objc_make_protocols_equal(Protocol *p1, Protocol *p2)
{
#define COPY(x) p1->x = p2->x
	COPY(instance_methods);
	COPY(class_methods);
	COPY(protocols);
	COPY(optional_instance_methods);
	COPY(optional_class_methods);
	COPY(properties);
	COPY(optional_properties);
#undef COPY
}

static Protocol *
_objc_unique_protocol(Protocol *aProto)
{
	if (objc_protocol_class == Nil){
		objc_protocol_class = (Class)objc_getClass("Protocol");
	}
	
	Protocol *oldProtocol = objc_protocol_table_get(objc_protocols,
													aProto->name);
	if (NULL == oldProtocol){
		/*
		 * This is the first time we've seen this protocol, so add it
		 * to the hash table and ignore it.
		 */
		objc_protocol_insert(objc_protocols, aProto);
		return aProto;
	}
	
	if (_objc_protocol_is_empty(oldProtocol)){
		if (_objc_protocol_is_empty(aProto)){
			return aProto;
			/* Add protocol to a list somehow. */
		}else{
			/*
			 * This protocol is not empty, so we use its
			 * definitions
			 */
			_objc_make_protocols_equal(oldProtocol, aProto);
			return aProto;
		}
	}else{
		if (_objc_protocol_is_empty(aProto)){
			_objc_make_protocols_equal(aProto, oldProtocol);
			return oldProtocol;
		}else{
			return oldProtocol;
		}
	}
}

enum objc_protocol_version {
	PROTOCOL_KERN_OBJC_VERSION = 0x100
};

static BOOL
_objc_init_protocols(objc_protocol_list *protocols)
{
	if (Nil == objc_protocol_class){
		return NO;
	}
	
	for (unsigned i=0 ; i<protocols->size ; i++)
	{
		Protocol *aProto = protocols->list[i];
		/* Don't initialise a protocol twice */
		if (aProto->isa == objc_protocol_class) { continue; }
		
		/*
		 * Protocols in the protocol list have their class pointers set
		 * to the version of the protocol class that they expect.
		 */
		objc_assert((enum objc_protocol_version)aProto->isa
					== PROTOCOL_KERN_OBJC_VERSION,
					"Protocol of unknown version %p!\n", aProto->isa);
		
		/*
		 * Initialize all of the protocols that this protocol refers to
		 */
		if (NULL != aProto->protocols){
			_objc_init_protocols(aProto->protocols);
		}
		
		/* Replace this protocol with a unique version of it. */
		protocols->list[i] = _objc_unique_protocol(aProto);
	}
	return YES;
}

#pragma mark -
#pragma mark Private Export

PRIVATE void
objc_init_protocols(objc_protocol_list *protocols)
{
	if (!_objc_init_protocols(protocols))
	{
		set_buffered_object_at_index(protocols, buffered_objects++);
		return;
	}
	if (buffered_objects > 0) { return; }
	
	/* If we can load one protocol, then we can load all of them. */
	for (unsigned i=0 ; i<buffered_objects ; i++){
		objc_protocol_list *c = buffered_object_at_index(i);
		if (NULL != c){
			_objc_init_protocols(c);
			set_buffered_object_at_index(NULL, i);
		}
	}
	compact_buffer();
}



#pragma mark -
#pragma mark Public Functions

Protocol *
objc_getProtocol(const char *name)
{
	if (name == NULL){
		return NULL;
	}
	
	objc_debug_log("Getting protocol %s\n", name);
	for (unsigned int i = 0; i < objc_protocols->table_size; ++i){
		if (objc_protocols->table[i].value != NULL){
			objc_debug_log("\t\t protocol [%02d] --> %p [%s]\n", i,
						   objc_protocols->table[i].value,
						   objc_protocols->table[i].value->name);
		}
	}
	
	
	return objc_protocol_table_get(objc_protocols, name);
}

BOOL
protocol_conformsToProtocol(Protocol *p1, Protocol *p2)
{
	if (p1 == NULL || p2 == NULL){
		return NO;
	}
	
	if (p1 == p2 || objc_strings_equal(p1->name, p2->name)){
		return YES;
	}
	
	objc_protocol_list *list = p1->protocols;
	while (list != NULL) {
		for (int i = 0; i < list->size; ++i){
			Protocol *p = list->list[i];
			if (p == p2 || objc_strings_equal(p->name, p2->name)){
				return YES;
			}
			if (protocol_conformsToProtocol(p, p2)){
				return YES;
			}
		}
		
		list = list->next;
	}
	return NO;
	
}

BOOL
class_addProtocol(Class cls, Protocol *protocol)
{
	if (cls == Nil || protocol == NULL){
		return NO;
	}
	if (class_conformsToProtocol(cls, protocol)){
		return NO;
	}
	
	objc_protocol_list *list = objc_protocol_list_create(1);
	list->list[0] = protocol;
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	list->next = cls->protocols;
	cls->protocols = list;
	return YES;
}

BOOL
class_conformsToProtocol(Class cls, Protocol *protocol)
{
	if (cls == Nil || protocol == NULL){
		return NO;
	}
	
	objc_protocol_list *list = cls->protocols;
	while (list != NULL) {
		for (int i = 0; i < list->size; ++i){
			if (protocol_conformsToProtocol(list->list[i], protocol)){
				return YES;
			}
		}
		
		list = list->next;
	}
	return NO;
}

Protocol * __unsafe_unretained *
class_copyProtocolList(Class cls, unsigned int *outCount)
{
	if (cls == Nil || cls->protocols == NULL || cls->protocols->size == 0){
		if (outCount != NULL){
			*outCount = 0;
		}
		return NULL;
	}
	return objc_protocol_list_copy_list(cls->protocols, outCount);
}

static objc_method_description_list *
_objc_protocol_get_method_list(Protocol *p,
							   BOOL isRequiredMethod,
							   BOOL isInstanceMethod)
{
	objc_method_description_list **list_ptr;
	list_ptr = _objc_protocol_get_method_list_ptr(p, isRequiredMethod,
												  isInstanceMethod);
	return list_ptr == NULL ? NULL : *list_ptr;
}

struct objc_method_description *
protocol_copyMethodDescriptionList(Protocol *p, BOOL isRequiredMethod,
								   BOOL isInstanceMethod, unsigned int *count)
{
	objc_method_description_list *list;
	list = _objc_protocol_get_method_list(p, isRequiredMethod,
										  isInstanceMethod);
	if (list == NULL || list->size == 0){
		if (count != NULL){
			*count = 0;
		}
		return NULL;
	}
	
	if (count != NULL){
		*count = list->size;
	}
	
	struct objc_method_description *descriptions;
	descriptions = objc_zero_alloc(sizeof(struct objc_method_description)
								   * list->size, M_PROTOCOL_TYPE);
	for (int i = 0; i < list->size; ++i){
		descriptions[i] = list->list[i];
	}
	
	return descriptions;
}

Protocol*__unsafe_unretained*
protocol_copyProtocolList(Protocol *p, unsigned int *count)
{
	if (p == NULL || p->protocols == NULL || p->protocols->size == 0){
		if (count != NULL){
			*count = 0;
		}
		return NULL;
	}
	return objc_protocol_list_copy_list(p->protocols, count);
}

Property *
protocol_copyPropertyList(Protocol *protocol, unsigned int *outCount)
{
	if (outCount != NULL){
		*outCount = 0;
	}
	if (protocol == NULL){
		return NULL;
	}
	
	objc_property_list *list;
	objc_property_list *optional_list;
	
	list = *_objc_protocol_get_property_list_ptr(protocol, YES, YES);
	optional_list = *_objc_protocol_get_property_list_ptr(protocol, NO, YES);
	
	unsigned int list_size = list == NULL ? 0 : list->size;
	unsigned int optional_list_size = optional_list == NULL ? 0 : optional_list->size;
	unsigned int size = list_size + optional_list_size;
	
	if (size == 0){
		return NULL;
	}
	
	Property *buffer = objc_zero_alloc(size * sizeof(Property), M_PROTOCOL_TYPE);
	if (list != NULL){
		objc_property_list_get_list(list, buffer, list_size);
	}
	if (optional_list != NULL){
		objc_property_list_get_list(optional_list, buffer + list_size,
									optional_list_size);
	}
	
	if (outCount != NULL){
		*outCount = size;
	}
	
	return buffer;
}

Property
protocol_getProperty(Protocol *protocol,
					 const char *name,
					 BOOL required,
					 BOOL instance)
{
	objc_property_list **list_ptr;
	list_ptr = _objc_protocol_get_property_list_ptr(protocol, required,
													instance);
	if (list_ptr == NULL || *list_ptr == NULL){
		return NULL;
	}
	
	objc_property_list *list = *list_ptr;
	while (list != NULL) {
		for (int i = 0; i < list->size; ++i){
			if (objc_strings_equal(list->list[i].name, name)){
				return &list->list[i];
			}
		}
		list = list->next;
	}
	return NULL;
}

struct objc_method_description
protocol_getMethodDescription(Protocol *p, SEL aSel,
							  BOOL required, BOOL instance)
{
	
	struct objc_method_description result = {0,0};
	objc_method_description_list *list;
	list = _objc_protocol_get_method_list(p, required, instance);
	if (list == NULL){
		return result;
	}
	
	for (int i = 0; i < list->size; ++i){
		if (list->list[i].selector == aSel){
			return list->list[i];
		}
	}
	
	return result;
}

const char *
protocol_getName(Protocol *protocol)
{
	return protocol == NULL ? NULL : protocol->name;
}

BOOL
protocol_isEqual(Protocol *protocol1, Protocol *protocol2)
{
	objc_debug_log("%s - protocol1 : %p --- protocol2 : %p\n", __FUNCTION__,
				   protocol1, protocol2);
	
	if (protocol1 == NULL || protocol2 == NULL){
		return NO;
	}
	
	/*
	 * We can skip comparing pointers since objc_strings_equal does
	 * this anyway
	 */
	return objc_strings_equal(protocol1->name, protocol2->name);
}

Protocol*__unsafe_unretained*
objc_copyProtocolList(Protocol *protocol, unsigned int *outCount)
{
	return objc_protocol_list_copy_list(protocol->protocols, outCount);
}

Protocol *
objc_allocateProtocol(const char *name)
{
	if (objc_getProtocol(name) != NULL) {
		return NULL;
	}
	Protocol *p = objc_zero_alloc(sizeof(Protocol), M_PROTOCOL_TYPE);
	p->name = objc_strcpy(name);
	p->flags.user_created = YES;
	return p;
}

void
objc_registerProtocol(Protocol *protocol)
{
	if (protocol == NULL){
		return;
	}
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	if (objc_getProtocol(protocol->name) != NULL){
		return;
	}
	
	if (objc_protocol_class == Nil){
		objc_protocol_class = (Class)objc_getClass("Protocol");
	}
	
	protocol->isa = objc_protocol_class;
	objc_protocol_insert(objc_protocols, protocol);
}

void
protocol_addMethodDescription(Protocol *aProtocol,
							  SEL selector,
							  const char *types,
							  BOOL isRequiredMethod,
							  BOOL isInstanceMethod)
{
	if (aProtocol == NULL || selector == 0 || types == NULL){
		return;
	}
	
	const char *sel_types = sel_getTypes(selector);
	objc_assert(objc_strings_equal(sel_types, types),
				"Trying to add a method description to a protocol with "
				"types that are different than the ones of the selector.\n")
	
	objc_method_description_list **list_ptr;
	list_ptr = _objc_protocol_get_method_list_ptr(aProtocol,
												  isRequiredMethod,
												  isInstanceMethod);
	if (*list_ptr == NULL){
		*list_ptr = objc_method_description_list_create(1);
	}else{
		*list_ptr = objc_method_description_list_expand_by(*list_ptr, 1);
	}
	
	struct objc_method_description *m;
	m = &(*list_ptr)->list[(*list_ptr)->size - 1];
	m->selector = selector;
	m->selector_types = types;
}

void
protocol_addProtocol(Protocol *aProtocol, Protocol *addition)
{
	if (aProtocol == NULL || addition == NULL){
		return;
	}
	
	if (aProtocol->protocols == NULL){
		aProtocol->protocols = objc_protocol_list_create(1);
	}else{
		aProtocol->protocols =
		objc_protocol_list_expand_by(aProtocol->protocols, 1);
	}
	
	aProtocol->protocols->list[aProtocol->protocols->size - 1] = addition;
}

void
protocol_addProperty(Protocol *protocol, const char *name,
					 const objc_property_attribute_t *atts,
					 unsigned int att_count, BOOL required, BOOL instance)
{
	/* Some basic check of the parameters */
	if (protocol == NULL || name == NULL || atts == NULL || instance == NO){
		return;
	}
	
	objc_property_list **list_ptr = required ? &protocol->properties
	: &protocol->optional_properties;
	if (*list_ptr == NULL){
		*list_ptr = objc_property_list_create(1);
	}else{
		*list_ptr = objc_property_list_expand_by(*list_ptr, 1);
	}
	
	const char *ivar_name = NULL;
	struct objc_property prop = propertyFromAttrs(atts, att_count,
												  &ivar_name);
	prop.name = name;
	constructPropertyAttributes(&prop, ivar_name);
	objc_copy_memory(&((*list_ptr)->list[(*list_ptr)->size - 1]),
					 &prop, sizeof(prop));
}


PRIVATE void
objc_protocol_init(void)
{
	objc_debug_log("Initializing protocols.\n");
	objc_protocols = objc_protocol_table_create(64, "objc_protocol_table");
}


static void
__objc_protocol_dealloc(Protocol *protocol)
{
	if (!protocol->flags.user_created){
		/* Don't free something that's not created by the user. */
		return;
	}
	
	objc_debug_log("Deallocating protocol %s\n", protocol->name);
	
	objc_method_description_list_free(protocol->class_methods);
	objc_method_description_list_free(protocol->instance_methods);
	objc_method_description_list_free(protocol->optional_class_methods);
	objc_method_description_list_free(protocol->optional_instance_methods);
	objc_property_list_free(protocol->optional_properties);
	objc_property_list_free(protocol->properties);
	objc_protocol_list_free(protocol->protocols);
	
	objc_dealloc(protocol, M_PROTOCOL_TYPE);
}

PRIVATE void
objc_protocol_unload(Protocol *protocol)
{
	objc_protocol_remove(objc_protocols, (void*)protocol->name);
	__objc_protocol_dealloc(protocol);
}


PRIVATE void
objc_protocol_destroy(void)
{
	objc_debug_log("Destroying protocols.\n");
	objc_protocol_table_destroy(objc_protocols, __objc_protocol_dealloc);
}

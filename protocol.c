#include "types.h"
#include "utils.h"
#include "property.h"
#include "selector.h"
#include "runtime.h"

static inline BOOL _objc_protocols_are_equal(const char *name, Protocol *p){
	return objc_strings_equal(name, p->name);
}

static inline int _objc_hash_protocol(Protocol *p){
	return objc_hash_string(p->name);
}

#define MAP_TABLE_NAME objc_protocol
#define MAP_TABLE_COMPARE_FUNCTION _objc_protocols_are_equal
#define MAP_TABLE_VALUE_TYPE Protocol *
#define MAP_TABLE_HASH_KEY objc_hash_string
#define MAP_TABLE_HASH_VALUE _objc_hash_protocol
#include "hashtable.h"

static objc_protocol_table *objc_protocols;
static Class objc_protocol_class;



#pragma mark -
#pragma mark Public Functions

Protocol *objc_getProtocol(const char *name){
	return objc_protocol_table_get(objc_protocols, name);
}


const char *protocol_getName(Protocol *protocol){
	return protocol == NULL ? NULL : protocol->name;
}

BOOL protocol_isEqual(Protocol *protocol1, Protocol *protocol2){
	if (protocol1 == NULL || protocol2 == NULL){
		return NO;
	}
	
	return protocol1 == protocol2
		|| protocol1->name == protocol2->name
		|| objc_strings_equal(protocol1->name, protocol2->name);
}

Protocol*__unsafe_unretained* objc_copyProtocolList(Protocol *protocol, unsigned int *outCount){
	return objc_protocol_list_copy_list(protocol->protocols, outCount);
}

Protocol *objc_allocateProtocol(const char *name){
	if (objc_getProtocol(name) != NULL) {
		return NULL;
	}
	Protocol *p = objc_zero_alloc(sizeof(Protocol));
	p->name = objc_strcpy(name);
	return p;
}

void objc_protocol_register(Protocol *protocol){
	if (protocol == NULL){
		return;
	}
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	
	if (objc_getProtocol(protocol->name) != NULL){
		return;
	}
	
	protocol->isa = objc_protocol_class;
	objc_protocol_insert(objc_protocols, protocol);
}

void objc_protocol_add_method(Protocol *aProtocol,
                                   SEL selector,
                                   const char *types,
                                   BOOL isRequiredMethod,
                                   BOOL isInstanceMethod){
	if (aProtocol == NULL || selector == 0 || types == NULL){
		return;
	}
	
	objc_assert(objc_strings_equal(objc_selector_get_types(selector), types), "Trying to add a method description to a protocol with types that are different than the ones of the selector.\n")
	
	objc_method_list **list_ptr = (isRequiredMethod ?
				       (isInstanceMethod ? &aProtocol->instance_methods : &aProtocol->class_methods) :
				       (isInstanceMethod ? &aProtocol->optional_instance_methods : &aProtocol->optional_class_methods)
				       );
	if (*list_ptr == NULL){
		*list_ptr = objc_method_list_create(1);
	}else{
		*list_ptr = objc_method_list_expand_by(*list_ptr, 1);
	}
	
	Method m = &(*list_ptr)->method_list[(*list_ptr)->size - 1];
	m->selector = selector;
	m->selector_types = types;
	m->selector_name = objc_selector_get_name(selector);
}

void objc_protocol_add_protocol(Protocol *aProtocol, Protocol *addition){
	if (aProtocol == NULL || addition == NULL){
		return;
	}
	
	if (aProtocol->protocols == NULL){
		aProtocol->protocols = objc_protocol_list_create(1);
	}else{
		aProtocol->protocols = objc_protocol_list_expand_by(aProtocol->protocols, 1);
	}
	
	aProtocol->protocols->protocol_list[aProtocol->protocols->size - 1] = addition;
}

void objc_protocol_add_property(Protocol *protocol, const char *name,
						  const objc_property_attribute_t *atts,
						  unsigned int att_count, BOOL required, BOOL instance){
	// Some basic check of the parameters
	if (protocol == NULL || name == NULL || atts == NULL || instance == NO){
		return;
	}
	
	objc_property_list **list_ptr = required ? &protocol->properties : &protocol->optional_properties;
	if (*list_ptr == NULL){
		*list_ptr = objc_property_list_create(1);
	}else{
		*list_ptr = objc_property_list_expand_by(*list_ptr, 1);
	}
	
	const char *ivar_name = NULL;
	struct objc_property prop = propertyFromAttrs(atts, att_count, &ivar_name);
	prop.name = name;
	constructPropertyAttributes(&prop, ivar_name);
	objc_copy_memory(&((*list_ptr)->property_list[(*list_ptr)->size - 1]), &prop, sizeof(prop));
}


PRIVATE void objc_protocol_init(void){
	objc_protocols = objc_protocol_table_create(64);
}

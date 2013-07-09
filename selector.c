
#include "selector.h"
#include "utils.h" /* For strcpy */

/**
 * Since the SEL is a 16-bit integer, all we need to
 * have are 2-level sparse arrays that get slowly filled,
 * the index in the sparse array is the actual SEL which
 * means quick SEL -> selector name conversion.
 *
 * To efficiently register selectors, however, it is necessary
 * to keep an additional hash table that hashes name -> SEL.
 *
 */


// TODO initialize
static void *selector_hashtable;
static void *selector_sparse;

static void objc_selector_register_direct(Selector selector) {
	objc_assert(selector != NULL, "Registering NULL selector!");
	
	// TODO
	// Actually register
}


/* Public functions, documented in the header file. */
SEL objc_selector_register(const char *name const char *types){
	objc_assert(name != NULL, "Cannot register a selector with NULL name!");
	objc_assert(types != NULL, "Cannot register a selector with NULL types!");
	objc_assert(objc_strlen(types) > 2, "Not enough types for registering selector.");
	
	Selector selector = objc_selector_hashtable_lookup(selector_hashtable, name);
	if (selector == NULL){
		selector = objc_alloc(sizeof(struct objc_selector));
		selector->name = objc_strcpy(name);
		sel_to_add->types = objc_strcpy(types);
		selector->selUID = 0; // Will be populated when registered
		
		objc_selector_register_direct(selector);
	}else{
		objc_assert(objc_strings_equal(types, selector->types), "Trying to register a"
			    " selector with the same name but different types!");
	}
	
	return selector->selUID;
}

const char *objc_selector_get_name(SEL selector){
	if (selector == NULL){
		return "((null))";
	}
	
	Selector selector = objc_selector_hashtable_lookup(selector_cache, name);
	objc_assert(selector != NULL, "Trying to get name from an unregistered selector.");
	return selector->name;
}

const char *objc_selector_get_types(SEL selector){
	if (selector == NULL){
		return "";
	}
	Selector selector = objc_selector_hashtable_lookup(selector_cache, name);
	objc_assert(selector != NULL, "Trying to get types from an unregistered selector.");
	return selector->types;
}

void objc_selector_init(void) __attribute__((constructor));
void objc_selector_init(void){
	// TODO init table + sparse
}


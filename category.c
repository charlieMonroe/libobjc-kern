#include "types.h"
#include "selector.h"
#include "dtable.h"
#include "class.h"
#include "protocol.h"
#include "private.h"

#define BUFFER_TYPE struct objc_category
#include "buffer.h"


/*
 * Registers the method list with class.
 */
static void
_objc_register_methods(Class cls, objc_method_list *list)
{
	if (NULL == list) { return; }
	
	/* Replace the method names with selectors. */
	objc_register_selectors_from_method_list(list);
	
	/* Add the method list at the head of the list of lists. */
	list->next = cls->methods;
	cls->methods = list;
	
	/*
	 * Update the dtable to catch the new methods, if the dtable has been
	 * created (don't bother creating dtables for classes when categories
	 * are loaded if the class hasn't received any messages yet.
	 */
	if (classHasDtable(cls)){
		dtable_add_method_list_to_class(cls, list);
	}
}

/*
 * Registers methods from the category with the class.
 * The same with protocols.
 */
static void
_objc_category_load(Category cat, Class class)
{
	_objc_register_methods(class, cat->instance_methods);
	_objc_register_methods(class->isa, cat->class_methods);
	
	if (cat->protocols){
		objc_init_protocols(cat->protocols);
		cat->protocols->next = class->protocols;
		class->protocols = cat->protocols;
	}
}

static BOOL
_objc_category_try_load(Category category)
{
	Class cl = (Class)objc_getClass(category->class_name);
	if (cl == Nil){
		return NO;
	}
	
	_objc_category_load(category, cl);
	return YES;
}

/*
 * Sees whether the class for the category was loaded yet,
 * and if so, attaches all the methods and protocols to the class.
 * If not, puts the category to the buffer.
 */
PRIVATE void
objc_category_try_load(Category category)
{
	objc_assert(category != NULL, "Trying to load a NULL category.\n");
	
	if (!_objc_category_try_load(category)){
		// Save it for later
		set_buffered_object_at_index(category, buffered_objects++);
	}
}

PRIVATE void
objc_load_buffered_categories(void)
{
	BOOL any_category_loaded = NO;
	
	for (unsigned i = 0; i < buffered_objects; ++i){
		Category c = buffered_object_at_index(i);
		if (NULL != c){
			if (_objc_category_try_load(c)){
				set_buffered_object_at_index(NULL, i);
				any_category_loaded = YES;
			}
		}
	}
	
	if (any_category_loaded){
		compact_buffer();
	}
}


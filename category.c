#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "selector.h"
#include "kernobjc/runtime.h"
#include "sarray2.h"
#include "dtable.h"
#include "class.h"
#include "kernobjc/protocol.h"
#include "private.h"

#define BUFFER_TYPE struct objc_category
#include "buffer.h"


/*
 * Registers the method list with class.
 */
static void
_objc_register_methods(Class cls, objc_method_list *list)
{
	if (NULL == list || list->size == 0) { return; }
	
	/*
	 * We need to create a copy of the method list. See the unload test. When
	 * a module implements a category on a class that's implemented on another
	 * module, then the module gets unloaded, unloading the other module will
	 * end up in a kernel panic as the memory of this list had been unloaded.
	 *
	 * We don't need to copy the strings, etc. as they get updated on the module
	 * unload.
	 */
	
	size_t size = sizeof(objc_method_list) +
							(list->size * sizeof(struct objc_method));
	objc_method_list *copied_list = objc_alloc(size, M_METHOD_LIST_TYPE);
	memcpy(copied_list, list, size);
	copied_list->is_dynamically_allocated = YES;
	
	/* Replace the method names with selectors. */
	objc_register_selectors_from_method_list(copied_list);
	
	/* Add the method list at the head of the list of lists. */
	copied_list->next = cls->methods;
	cls->methods = copied_list;
	
	/*
	 * Update the dtable to catch the new methods, if the dtable has been
	 * created (don't bother creating dtables for classes when categories
	 * are loaded if the class hasn't received any messages yet.
	 */
	if (classHasDtable(cls)){
		dtable_add_method_list_to_class(cls, copied_list);
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
		/* Save it for later */
		set_buffered_object_at_index(category, buffered_objects++);
	}
}



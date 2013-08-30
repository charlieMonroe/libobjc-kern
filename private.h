
#ifndef OBJC_PRIVATE_H
#define OBJC_PRIVATE_H

/*
 * These functions are either marked as unavailble with ARC,
 * or indeed private.
 */

#pragma mark -
#pragma mark ARC_UNAVAILABLE

id	class_createInstance(Class cl, size_t extraBytes);
id	object_copy(id obj, size_t size);
void	object_dispose(id obj);
void	*object_getIndexedIvars(id obj);


#pragma mark -
#pragma mark Private

PRIVATE BOOL			objc_register_small_object_class(Class cl, uintptr_t mask);
PRIVATE void			objc_category_try_load(Category category);
PRIVATE struct objc_slot	*objc_get_slot(Class cl, SEL selector);
PRIVATE BOOL			objc_class_resolve(Class cl);
PRIVATE void			objc_updateDtableForClassContainingMethod(Method m);
PRIVATE	size_t			lengthOfTypeEncoding(const char *types);
PRIVATE void			objc_init_protocols(objc_protocol_list *protocols);
PRIVATE IMP				slowMsgLookup(id *receiver, SEL cmd);
PRIVATE void			objc_class_resolve_links(void);
PRIVATE Class			objc_class_get_root_class_list(void);
PRIVATE void			objc_unload_class(Class cl);
PRIVATE void			objc_protocol_unload(Protocol *protocol);

PRIVATE void
objc_load_buffered_categories(void);

PRIVATE void
objc_class_send_load_messages(Class cl);

struct objc_slot *
objc_msg_lookup_sender_non_nil(id *receiver, SEL selector, id sender);
PRIVATE struct objc_slot *
objc_slot_lookup_super(struct objc_super *super, SEL selector);

id
objc_lookup_class(const char *name);

PRIVATE void
objc_class_resolve_links(void);

PRIVATE void
call_cxx_construct(id obj);
PRIVATE void
call_cxx_destruct(id obj);

#endif


#ifndef OBJC_PRIVATE_H
#define OBJC_PRIVATE_H

/*
 * These functions are private, meant generally only for private runtime use.
 */

/* Registers a small object class (aka tagged pointers). */
PRIVATE BOOL			objc_register_small_object_class(Class cl,
														 uintptr_t mask);

/* Tries to load a category. */
PRIVATE void			objc_category_try_load(Category category);

/* Gets a slot for class. May return NULL. */
PRIVATE struct objc_slot	*objc_get_slot(Class cl, SEL selector);

/* Attempts to resolve a class. */
PRIVATE BOOL			objc_class_resolve(Class cl);

/* Updates dtables of classes containing this method. */
PRIVATE void			objc_updateDtableForClassContainingMethod(Method m);

/* Returns length of type encoding. */
PRIVATE	size_t			lengthOfTypeEncoding(const char *types);

/* Inits a list of protocols. Used from categories. */
PRIVATE void			objc_init_protocols(objc_protocol_list *protocols);

/* A function that gets called if the IMP isn't found in dtable in objc_msgSend 
 */
PRIVATE IMP				slowMsgLookup(id *receiver, SEL cmd);

/*
 * Returns the first root class registered with the runtime, i.e. the root
 * of the class tree. Used for climbing the class tree.
 */
PRIVATE Class			objc_class_get_root_class_list(void);

/* Unloads a class. */
PRIVATE void			objc_class_unload(Class cl);

/* Unloads a protocol. */
PRIVATE void			objc_protocol_unload(Protocol *protocol);

/* 
 * Prints out a list of classes registered with the runtime. Mostly for debug
 * purposes.
 */
void objc_classes_dump(void);

/* Looks up a slot. */
struct objc_slot *objc_msg_lookup_sender_non_nil(id *receiver, SEL selector,
												 id sender);

/* Looks up a slot on super. */
PRIVATE struct objc_slot *objc_slot_lookup_super(struct objc_super *super,
												 SEL selector);

/* This is the function that currently gets called to get the Class pointer
 * for any class methods. Hopefully this will change soon, but requires compiler
 * support.
 */
PRIVATE id objc_lookup_class(const char *name);

/* Resolves all unresolved class links if possible. */
PRIVATE void objc_class_resolve_links(void);


#pragma mark -
#pragma mark Default hooks

/*
 * These are the default hooks that the hooks in kernobjc/hooks.h are declared.
 */
extern struct objc_slot		*objc_msg_forward3_default(id receiver, SEL op);
extern id					objc_proxy_lookup_default(id receiver, SEL op);
extern struct objc_slot		*objc_msg_lookup_default(id *receiver, SEL selector,
													 id sender);
extern Class				__objc_class_lookup_default_hook(const char *name);
extern void					__objc_unloaded_module_implementation_called(id sender,
																		 SEL _cmd);
#endif

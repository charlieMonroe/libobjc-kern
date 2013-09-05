
#ifndef LIBKERNOBJC_RUNTIME_H
#define LIBKERNOBJC_RUNTIME_H

#include "types.h"
#include "selector.h"
#include "object.h"
#include "class.h"
#include "ivar.h"
#include "method.h"
#include "property.h"
#include "protocol.h"
#include "message.h"
#include "arc.h"
#include "hooks.h"
#include "encoding.h"
#include "loader.h"
#ifdef __OBJC__
	#import "KKObjects.h"
#endif

/*
 * Finds a class registered with the run-time and returns it,
 * or Nil, if no such class is registered.
 *
 * Before returning Nil, however, it checks the hook
 * if it can supply a class (unlike the objc_lookUpClass()).
 */
id objc_getClass(const char *name);
id objc_getMetaClass(const char *name);
id objc_lookUpClass(const char *name);

/*
 * Unlike the previous functions, aborts if the class
 * cannot be found.
 */
id objc_getRequiredClass(const char *name);

/*
 * Returns a list of classes registered with the run-time. 
 * The caller is responsible for freeing
 * it using the free function.
 */
int objc_getClassList(Class *buffer, int bufferCount);
Class *objc_copyClassList(unsigned int *outCount);

Protocol *objc_getProtocol(const char *name);
Protocol*__unsafe_unretained* objc_copyProtocolList(Protocol *protocol, unsigned int *outCount);

/*
 * Creates a new class with name that is a subclass of superclass.
 *
 * The returned class isn't registered with the run-time yet,
 * you need to use the objc_registerPair function.
 *
 * Memory management note: the name is copied.
 *
 * The regular version guesses the module by looking at the caller function 
 * pointer and trying to find the correct module. This, however, won't work if
 * you are using some other module's function for class creation - hence the
 * "InModule" alternative.
 */
Class objc_allocateClassPair(Class superclass, const char *name,
			     size_t extraBytes);
Class objc_allocateClassPairInModule(Class superclass, const char *name,
									 size_t extraBytes, void *module);

/*
 * This function marks the class as finished (i.e. resolved, not in construction).
 * You need to call this before creating any instances.
 */
void objc_registerClassPair(Class cls);

/*
 * Removes class from the run-time and releases the class structures.
 */
void objc_disposeClassPair(Class cls);




void objc_enumerationMutation(id);
void objc_setEnumerationMutationHandler(void (*handler)(id));

void objc_setForwardHandler(void *fwd, void *fwd_stret);

IMP imp_implementationWithBlock(id block);
id imp_getBlock(IMP anImp);
BOOL imp_removeBlock(IMP anImp);


/* Associated Object support. */
void objc_setAssociatedObject(id object, const void *key, id value, objc_AssociationPolicy policy);
id objc_getAssociatedObject(id object, const void *key);
void objc_removeAssociatedObjects(id object);


id objc_loadWeak(id *location);
id objc_storeWeak(id *location, id obj);

int objc_sync_enter(id obj);
int objc_sync_exit(id obj);


#endif /* !LIBKERNOBJC_RUNTIME_H */

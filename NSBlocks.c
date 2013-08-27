#include "kernobjc/runtime.h"
#include "types.h"
#include "init.h"
#include "dtable.h"
#include "runtime.h"
#include "class_registry.h"

struct objc_class _NSConcreteGlobalBlock;
struct objc_class _NSConcreteStackBlock;
struct objc_class _NSConcreteMallocBlock;

static struct objc_class _NSConcreteGlobalBlockMeta;
static struct objc_class _NSConcreteStackBlockMeta;
static struct objc_class _NSConcreteMallocBlockMeta;

static struct objc_class _NSBlock;
static struct objc_class _NSBlockMeta;

static void createNSBlockSubclass(Class superclass, Class newClass,
								  Class metaClass, char *name)
{
	// Initialize the metaclass
	metaClass->isa = superclass->isa;
	metaClass->super_class = superclass->isa;
	metaClass->flags.meta = YES;
	metaClass->dtable = uninstalled_dtable;
	
	// Set up the new class
	newClass->isa = metaClass;
	newClass->super_class = (Class)superclass->name;
	newClass->name = name;
	newClass->dtable = uninstalled_dtable;
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	objc_class_register_class(newClass);
	
}

#define NEW_CLASS(super, sub) \
createNSBlockSubclass(super, &sub, &sub ## Meta, #sub)

static BOOL objc_create_block_classes_as_subclasses_of(Class super)
{
	if (_NSBlock.super_class != NULL) { return NO; }
	
	NEW_CLASS(super, _NSBlock);
	NEW_CLASS(&_NSBlock, _NSConcreteStackBlock);
	NEW_CLASS(&_NSBlock, _NSConcreteGlobalBlock);
	NEW_CLASS(&_NSBlock, _NSConcreteMallocBlock);
	return YES;
}


PRIVATE void
objc_blocks_init(void)
{
	objc_create_block_classes_as_subclasses_of((Class)objc_getClass("KKObject"));
}

PRIVATE void
objc_blocks_destroy(void)
{
	// No-op
}


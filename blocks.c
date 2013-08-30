#include "kernobjc/runtime.h"
#include "types.h"
#include "init.h"
#include "sarray2.h"
#include "dtable.h"
#include "runtime.h"
#include "class_registry.h"
#include "blocks.h"
#include "private.h"

static void *_HeapBlockByRef = (void*)1;

struct objc_class _NSConcreteGlobalBlock;
struct objc_class _NSConcreteStackBlock;
struct objc_class _NSConcreteMallocBlock;

#pragma mark Runtime

/**
 * Returns the Objective-C type encoding for the block.
 */
const char *block_getType_np(void *b)
{
	struct Block_layout *block = b;
	if ((NULL == block) || !(block->flags & BLOCK_HAS_SIGNATURE))
	{
		return NULL;
	}
	if (!(block->flags & BLOCK_HAS_COPY_DISPOSE))
	{
		return ((struct Block_descriptor_basic*)block->descriptor)->encoding;
	}
	return block->descriptor->encoding;
}

static int increment24(int *ref)
{
	int old = *ref;
	int val = old & BLOCK_REFCOUNT_MASK;
	// FIXME: We should gracefully handle refcount overflow, but for now we
	// just give up
	objc_assert(val < BLOCK_REFCOUNT_MASK, "Overflow\n");
	if (!__sync_bool_compare_and_swap(ref, old, old+1))
	{
		return increment24(ref);
	}
	return val + 1;
}

static int decrement24(int *ref)
{
	int old = *ref;
	int val = old & BLOCK_REFCOUNT_MASK;
	// FIXME: We should gracefully handle refcount overflow, but for now we
	// just give up
	objc_assert(val > 0, "Underflow");
	if (!__sync_bool_compare_and_swap(ref, old, old-1))
	{
		return decrement24(ref);
	}
	return val - 1;
}

/* Certain field types require runtime assistance when being copied to the
 * heap.  The following function is used to copy fields of types: blocks,
 * pointers to byref structures, and objects (including
 * __attribute__((NSObject)) pointers.  BLOCK_FIELD_IS_WEAK is orthogonal to
 * the other choices which are mutually exclusive.  Only in a Block copy helper
 * will one see BLOCK_FIELD_IS_BYREF.
 */
void _Block_object_assign(void *destAddr, const void *object, const int flags)
{
	//printf("Copying %x to %x with flags %x\n", object, destAddr, flags);
	// FIXME: Needs to be implemented
	//if(flags & BLOCK_FIELD_IS_WEAK)
	{
	}
	//else
	{
		if (IS_SET(flags, BLOCK_FIELD_IS_BYREF)){
			struct block_byref_obj *src = (struct block_byref_obj *)object;
			struct block_byref_obj **dst = destAddr;
			src = src->forwarding;
			
			if ((src->flags & BLOCK_REFCOUNT_MASK) == 0){
				*dst = objc_alloc(src->size, M_BLOCKS_TYPE);
				memcpy(*dst, src, src->size);
				(*dst)->isa = _HeapBlockByRef;
				
				// Refcount must be two; one for the copy and one for the
				// on-stack version that will point to it.
				(*dst)->flags += 2;
				if (IS_SET(src->flags, BLOCK_HAS_COPY_DISPOSE)){
					src->byref_keep(*dst, src);
				}
				
				(*dst)->forwarding = *dst;
				
				// Concurrency.  If we try copying the same byref structure
				// from two threads simultaneously, we could end up with two
				// versions on the heap that are unaware of each other.  That
				// would be bad.  So we first set up the copy, then try to do
				// an atomic compare-and-exchange to point the old version at
				// it.  If the forwarding pointer in src has changed, then we
				// recover - clean up and then return the structure that the
				// other thread created.
				if (!__sync_bool_compare_and_swap(&src->forwarding, src, *dst)) {
					if((size_t)src->size >= sizeof(struct block_byref_obj)){
						src->byref_dispose(*dst);
					}
					objc_dealloc(*dst, M_BLOCKS_TYPE);
					*dst = src->forwarding;
				}
			}else{
				*dst = (struct block_byref_obj*)src;
				increment24(&(*dst)->flags);
			}
		}else if (IS_SET(flags, BLOCK_FIELD_IS_BLOCK)){
			struct Block_layout *src = (struct Block_layout*)object;
			struct Block_layout **dst = destAddr;
			
			*dst = _Block_copy(src);
		}else if (IS_SET(flags, BLOCK_FIELD_IS_OBJECT) &&
		         !IS_SET(flags, BLOCK_BYREF_CALLER)){
			id src = (id)object;
			void **dst = destAddr;
			*dst = src;
			*dst = objc_retain(src);
		}
	}
}


/* Similarly a compiler generated dispose helper needs to call back for each
 * field of the byref data structure.  (Currently the implementation only packs
 * one field into the byref structure but in principle there could be more).
 * The same flags used in the copy helper should be used for each call
 * generated to this function:
 */
void _Block_object_dispose(const void *object, const int flags)
{
	// FIXME: Needs to be implemented
	//if(flags & BLOCK_FIELD_IS_WEAK)
	{
	}
	//else
	{
		if (IS_SET(flags, BLOCK_FIELD_IS_BYREF)){
			struct block_byref_obj *src = (struct block_byref_obj*)object;
			src = src->forwarding;
			if (src->isa == _HeapBlockByRef){
				int refcount = (src->flags & BLOCK_REFCOUNT_MASK) == 0 ? 0 : decrement24(&src->flags);
				if (refcount == 0){
					if(IS_SET(src->flags, BLOCK_HAS_COPY_DISPOSE) && (0 != src->byref_dispose)){
						src->byref_dispose(src);
					}
					objc_dealloc(src, M_BLOCKS_TYPE);
				}
			}
		}else if (IS_SET(flags, BLOCK_FIELD_IS_BLOCK)){
			struct Block_layout *src = (struct Block_layout*)object;
			_Block_release(src);
		}else if (IS_SET(flags, BLOCK_FIELD_IS_OBJECT) &&
				  !IS_SET(flags, BLOCK_BYREF_CALLER)) {
			id src = (id)object;
			objc_release(src);
		}
	}
}

// Copy a block to the heap if it's still on the stack or increments its retain count.
void *_Block_copy(void *src){
	if (NULL == src) { return NULL; }
	struct Block_layout *self = src;
	struct Block_layout *ret = self;
	
	// If the block is Global, there's no need to copy it on the heap.
	if(self->isa == &_NSConcreteStackBlock){
		ret = objc_alloc(self->descriptor->size, M_BLOCKS_TYPE);
		memcpy(ret, self, self->descriptor->size);
		ret->isa = &_NSConcreteMallocBlock;
		if(self->flags & BLOCK_HAS_COPY_DISPOSE){
			self->descriptor->copy_helper(ret, self);
		}
		
		// We don't need any atomic operations here, because on-stack blocks
		// can not be aliased across threads (unless you've done something
		// badly wrong).
		ret->reserved = 1;
	}else if (self->isa == &_NSConcreteMallocBlock){
		// We need an atomic increment for malloc'd blocks, because they may be
		// shared.
		__sync_fetch_and_add(&ret->reserved, 1);
	}
	return ret;
}

// Release a block and frees the memory when the retain count hits zero.
void _Block_release(void *src)
{
	if (NULL == src) { return; }
	struct Block_layout *self = src;
	
	if (&_NSConcreteStackBlock == self->isa){
		objc_log("Block_release called upon a stack Block: %p, ignored\n", self);
	}else if (&_NSConcreteMallocBlock == self->isa){
		if (__sync_sub_and_fetch(&self->reserved, 1) == 0){
			if(self->flags & BLOCK_HAS_COPY_DISPOSE)
				self->descriptor->dispose_helper(self);
			objc_delete_weak_refs((id)self);
			objc_dealloc(self, M_BLOCKS_TYPE);
		}
	}
}

PRIVATE void* block_load_weak(void *block)
{
	struct Block_layout *self = block;
	return (self->reserved) > 0 ? block : NULL;
}


#pragma mark -
#pragma mark Initialization

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
	metaClass->name = name;
	
	// Set up the new class
	newClass->isa = metaClass;
	newClass->super_class = superclass;
	newClass->name = name;
	newClass->dtable = uninstalled_dtable;
	
	objc_class_register_class(newClass);
	
}

#define NEW_CLASS(super, sub) \
createNSBlockSubclass(super, &sub, &sub ## Meta, #sub)

static BOOL objc_create_block_classes_as_subclasses_of(Class super)
{
	objc_assert(super != Nil, "Trying to initalize block classes with"
							" a Nil super!\n");
	
	if (_NSBlock.super_class != NULL) { return NO; }
	
	NEW_CLASS(super, _NSBlock);
	NEW_CLASS(&_NSBlock, _NSConcreteStackBlock);
	NEW_CLASS(&_NSBlock, _NSConcreteGlobalBlock);
	NEW_CLASS(&_NSBlock, _NSConcreteMallocBlock);
	
	/* Inserts the block classes to the class tree. */
	objc_class_resolve_links();
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


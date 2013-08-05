#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "sarray2.h"
#include "malloc_types.h"

static void *EmptyArrayData[256];
static SparseArray EmptyArray = { 0xff, 0, 0, (void**)&EmptyArrayData};
static void *EmptyArrayData8[256] = { [0 ... 255] = &EmptyArray };
static SparseArray EmptyArray8 = { 0xff00, 8, 0, (void**)&EmptyArrayData8};

#define MAX_INDEX(sarray) (sarray->mask >> sarray->shift)
#define DATA_SIZE(sarray) ((sarray->mask >> sarray->shift) + 1)

// Tweak this value to trade speed for memory usage.  Bigger values use more
// memory, but give faster lookups.  
#define base_shift 8
#define base_mask ((1<<base_shift) - 1)

static void *
EmptyChildForShift(uint16_t shift)
{
	switch(shift)
	{
		default: UNREACHABLE("Broken sparse array");
		case 8:
			return &EmptyArray;
		case 16:
			return &EmptyArray8;
	}
}

static void
init_pointers(SparseArray * sarray)
{
	sarray->data = objc_zero_alloc(DATA_SIZE(sarray) * sizeof(void*),
				       M_SPARSE_ARRAY_TYPE);
	if(sarray->shift != 0)
	{
		void *data = EmptyChildForShift(sarray->shift);
		for(unsigned i=0 ; i<=MAX_INDEX(sarray) ; i++)
		{
			sarray->data[i] = data;
		}
	}
}

PRIVATE SparseArray * SparseArrayNewWithDepth(uint16_t depth)
{
	SparseArray * sarray = objc_zero_alloc(sizeof(SparseArray),
					       M_SPARSE_ARRAY_TYPE);
	sarray->refCount = 1;
	sarray->shift = depth-base_shift;
	sarray->mask = base_mask << sarray->shift;
	init_pointers(sarray);
	return sarray;
}

PRIVATE SparseArray *SparseArrayNew(void)
{
	return SparseArrayNewWithDepth(16);
}
PRIVATE SparseArray *SparseArrayExpandingArray(SparseArray *sarray,
					       uint16_t new_depth)
{
	if (new_depth == sarray->shift)
	{
		return sarray;
	}
	objc_assert(new_depth > sarray->shift, "Wrong depth\n");
	// Expanding a child sarray has undefined results.
	objc_assert(sarray->refCount == 1, "Wrong refcount\n");
	SparseArray *new = objc_zero_alloc(sizeof(SparseArray), M_SPARSE_ARRAY_TYPE);
	new->refCount = 1;
	new->shift = sarray->shift;
	new->mask = sarray->mask;
	void **newData = objc_alloc(DATA_SIZE(sarray) * sizeof(void*),
				    M_SPARSE_ARRAY_TYPE);
	void *data = EmptyChildForShift(new->shift + 8);
	for(unsigned i=1 ; i<=MAX_INDEX(sarray) ; i++)
	{
		newData[i] = data;
	}
	new->data = sarray->data;
	newData[0] = new;
	sarray->data = newData;
	// Now, any lookup in sarray for any value less than its capacity will have
	// all non-zero values shifted away, resulting in 0.  All lookups will
	// therefore go to the new sarray.
	sarray->shift += base_shift;
	// Finally, set the mask to the correct value.  Now all lookups should work.
	sarray->mask <<= base_shift;
	return sarray;
}

static void *SparseArrayFind(SparseArray * sarray, uint16_t * index)
{
	uint16_t j = MASK_INDEX((*index));
	uint16_t max = MAX_INDEX(sarray);
	if (sarray->shift == 0)
	{
		while (j<=max)
		{
			if (sarray->data[j] != SARRAY_EMPTY)
			{
				return sarray->data[j];
			}
			(*index)++;
			j++;
		}
	}
	else while (j<max)
	{
		// If the shift is not 0, then we need to recursively look at child
		// nodes.
		uint16_t zeromask = ~(sarray->mask >> base_shift);
		while (j<max)
		{
			//Look in child nodes
			SparseArray *child = sarray->data[j];
			// Skip over known-empty children
			if ((&EmptyArray == child) ||
			    (&EmptyArray8 == child))
			{
				//Add 2^n to index so j is still correct
				(*index) += 1<<sarray->shift;
				//Zero off the next component of the index so we don't miss any.
				*index &= zeromask;
			}
			else
			{
				// The recursive call will set index to the correct value for
				// the next index, but won't update j
				void * ret = SparseArrayFind(child, index);
				if (ret != SARRAY_EMPTY)
				{
					return ret;
				}
			}
			//Go to the next child
			j++;
		}
	}
	return SARRAY_EMPTY;
}

PRIVATE void *SparseArrayNext(SparseArray * sarray, uint16_t * idx)
{
	(*idx)++;
	return SparseArrayFind(sarray, idx);
}

PRIVATE void SparseArrayInsert(SparseArray * sarray, uint16_t index, void *value)
{
	if (sarray->shift > 0)
	{
		uint16_t i = MASK_INDEX(index);
		SparseArray *child = sarray->data[i];
		if ((&EmptyArray == child) ||
		    (&EmptyArray8 == child))
		{
			// Insert missing nodes
			SparseArray * newsarray = objc_zero_alloc(sizeof(SparseArray),
								  M_SPARSE_ARRAY_TYPE);
			newsarray->refCount = 1;
			if (base_shift >= sarray->shift)
			{
				newsarray->shift = 0;
			}
			else
			{
				newsarray->shift = sarray->shift - base_shift;
			}
			newsarray->mask = sarray->mask >> base_shift;
			init_pointers(newsarray);
			sarray->data[i] = newsarray;
			child = newsarray;
		}
		else if (child->refCount > 1)
		{
			// Copy the copy-on-write part of the tree
			sarray->data[i] = SparseArrayCopy(child);
			SparseArrayDestroy(&child);
			child = sarray->data[i];
		}
		SparseArrayInsert(child, index, value);
	}
	else
	{
		sarray->data[index & sarray->mask] = value;
	}
}

PRIVATE SparseArray *SparseArrayCopy(SparseArray * sarray)
{
	SparseArray *copy = objc_zero_alloc(sizeof(SparseArray),
					    M_SPARSE_ARRAY_TYPE);
	copy->refCount = 1;
	copy->shift = sarray->shift;
	copy->mask = sarray->mask;
	copy->data = objc_alloc(sizeof(void*) * DATA_SIZE(sarray),
				M_SPARSE_ARRAY_TYPE);
	memcpy(copy->data, sarray->data, sizeof(void*) * DATA_SIZE(sarray));
	// If the sarray has children, increase their refcounts and link them
	if (sarray->shift > 0)
	{
		for (unsigned int i = 0 ; i<=MAX_INDEX(sarray); i++)
		{
			SparseArray *child = copy->data[i];
			__sync_fetch_and_add(&child->refCount, 1);
			// Non-lazy copy.  Uncomment if debugging 
			// copy->data[i] = SparseArrayCopy(copy->data[i]);
		}
	}
	return copy;
}

PRIVATE void SparseArrayDestroy(SparseArray **sarray)
{
	// Don't really delete this sarray if its ref count is > 0
	if (*sarray == &EmptyArray ||
	    *sarray == &EmptyArray8 ||
		(__sync_sub_and_fetch(&(*sarray)->refCount, 1) > 0))
 	{
		return;
	}

	if((*sarray)->shift > 0)
	{
		uint16_t max = ((*sarray)->mask >> (*sarray)->shift) + 1;
		for(uint16_t i=0 ; i<max ; i++)
		{
			SparseArrayDestroy((SparseArray**)&((*sarray)->data[i]));
		}
	}
	
	objc_dealloc((*sarray)->data, M_SPARSE_ARRAY_TYPE);
	objc_dealloc((*sarray), M_SPARSE_ARRAY_TYPE);
	*sarray = NULL;
}

PRIVATE int SparseArraySize(SparseArray *sarray)
{
	int size = 0;
	if (sarray->shift == 0)
	{
		return 256*sizeof(void*) + sizeof(SparseArray);
	}
	size += 256*sizeof(void*) + sizeof(SparseArray);
	for(unsigned i=0 ; i<=MAX_INDEX(sarray) ; i++)
	{
		SparseArray *child = sarray->data[i];
		if (child == &EmptyArray || 
		    child == &EmptyArray8)
		{
			continue;
		}
		size += SparseArraySize(child);
	}
	return size;
}

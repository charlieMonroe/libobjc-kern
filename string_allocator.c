#include "os.h"
#include "types.h"
#include "init.h"
#include "utils.h"

#define POOL_TYPE char
#define POOL_NAME objc_utilities
#define POOL_MALLOC_TYPE M_UTILITIES_TYPE
#define POOL_FREE_FORM_SIZE 1
#include "pool.h"


PRIVATE char *
objc_string_allocator_alloc(size_t size)
{
	return objc_utilities_pool_alloc(size);
}

PRIVATE void
objc_string_allocator_init(void)
{
	
}

PRIVATE void
objc_string_allocator_destroy(void)
{
	objc_utilities_pool_free();
}


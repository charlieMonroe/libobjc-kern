/**
 * hash_table.h provides a template for implementing hopscotch hash tables.
 *
 * Several macros must be defined before including this file:
 *
 * MAP_TABLE_NAME defines the name of the table.  All of the operations and
 * types related to this table will be prefixed with this value.
 *
 * MAP_TABLE_COMPARE_FUNCTION defines the function used for testing a key
 * against a value in the table for equality.  This must take two void*
 * arguments.  The first is the key and the second is the value.
 *
 * MAP_TABLE_HASH_KEY and MAP_TABLE_HASH_VALUE define a pair of functions that
 * takes a key and a value pointer respectively as their argument and returns
 * an int32_t representing the hash.
 *
 */

#include "os.h"

#ifndef MAP_TABLE_NAME
#	error You must define MAP_TABLE_NAME.
#endif
#ifndef MAP_TABLE_COMPARE_FUNCTION
#	error You must define MAP_TABLE_COMPARE_FUNCTION.
#endif
#ifndef MAP_TABLE_HASH_KEY
#	error You must define MAP_TABLE_HASH_KEY
#endif
#ifndef MAP_TABLE_HASH_VALUE
#	error You must define MAP_TABLE_HASH_VALUE
#endif

#ifndef MAP_PLACEHOLDER_VALUE
	#define MAP_PLACEHOLDER_VALUE NULL
#endif

#ifndef MAP_TABLE_KEY_TYPE
	#define MAP_TABLE_KEY_TYPE void *
#endif
#ifndef MAP_NULL_EQUALITY_FUNCTION
static inline int _ptr_is_null(void *ptr){
	return ptr == NULL;
}
#define MAP_NULL_EQUALITY_FUNCTION _ptr_is_null
#endif

/**
 * PREFIX(x) macro adds the table name prefix to the argument.
 */
#define PREFIX(x) PREFIX_SUFFIX(MAP_TABLE_NAME, x)

/**
 * Hash table cell. Includes the value + secondMaps
 * for collision handling - secondMaps is a bitmap -
 * if n-th bit is 1, then an item with this hash is also
 * at (hash + n).
 */
typedef struct PREFIX(_table_cell_struct)
{
	uint32_t secondMaps;
	MAP_TABLE_VALUE_TYPE value;
} *PREFIX(_table_cell);

/**
 * Actual table structure.
 *
 * RW lock for r/w locking, table_size for the allocated
 * table_size, table_used for number of items in the 
 * table, enumerator_count for counting enumerators (TODO),
 * old pointing to the old structure when rehashing (resizing),
 * table for the actual data.
 */
typedef struct PREFIX(_table_struct)
{
	objc_rw_lock lock;
	unsigned int table_size;
	unsigned int table_used;
	unsigned int enumerator_count;
	struct PREFIX(_table_struct) *old;
	struct PREFIX(_table_cell_struct) *table;
} PREFIX(_table);


/**
 * Allocating count cells for the table.
 */
struct PREFIX(_table_cell_struct) *PREFIX(alloc_cells)(int count)
{
	return objc_zero_alloc(count * sizeof(struct PREFIX(_table_cell_struct)));
}

/**
 * Allocates the table with initial capacity, initializes
 * the lock.
 */
PREFIX(_table) *PREFIX(_table_create)(uint32_t capacity)
{
	PREFIX(_table) *table = objc_zero_alloc(sizeof(PREFIX(_table)));
	objc_rw_lock_init(&table->lock);
	table->table = PREFIX(alloc_cells)(capacity);
	table->table_size = capacity;
	return table;
}

/**
 * Forward declaration.
 */
static int PREFIX(_insert)(PREFIX(_table) *table, MAP_TABLE_VALUE_TYPE value);


/**
 * Resizes the table by growing twice the current size.
 *
 * Returns 0 on failure, 1 on success.
 */
static int PREFIX(_table_resize)(PREFIX(_table) *table)
{
	struct PREFIX(_table_cell_struct) *newArray =
				PREFIX(alloc_cells)(table->table_size * 2);
	if (NULL == newArray) {
		return 0;
	}
	
	// Allocate a new table structure and move the array into that.  Now
	// lookups will try using that one, if possible.
	PREFIX(_table) *copy = objc_zero_alloc(sizeof(PREFIX(_table)));
	memcpy(copy, table, sizeof(PREFIX(_table)));
	table->old = copy;
	
	// Now we make the original table structure point to the new (empty) array.
	table->table = newArray;
	table->table_size *= 2;
	
	// The table currently has no entries; the copy has them all.
	table->table_used = 0;
	
	// Finally, copy everything into the new table
	// Note: we should really do this in a background thread.  At this stage,
	// we can do the updates safely without worrying about read contention.
	int copied = 0;
	for (uint32_t i=0 ; i<copy->table_size ; i++)
	{
		MAP_TABLE_VALUE_TYPE value = copy->table[i].value;
		if (!MAP_NULL_EQUALITY_FUNCTION(value))
		{
			copied++;
			PREFIX(_insert)(table, value);
		}
	}
	
	__sync_synchronize();
	table->old = NULL;
	free(copy->table);
	free(copy);
	
	return 1;
}

/**
 * Struct defining an enumerator.
 */
struct PREFIX(_table_enumerator)
{
	PREFIX(_table) *table;
	unsigned int seen;
	unsigned int index;
};


/**
 * Finds the table cell corresponding to the already
 * computed hash.
 */
static inline PREFIX(_table_cell) PREFIX(_table_lookup)(PREFIX(_table) *table,
                                                        uint32_t hash)
{
	hash = hash % table->table_size;
	return &table->table[hash];
}

/**
 * There is an empty space in cell at fromHash
 */
static int PREFIX(_table_move_gap)(PREFIX(_table) *table, uint32_t fromHash,
				   uint32_t toHash, PREFIX(_table_cell) emptyCell)
{
	for (uint32_t hash = fromHash - 32 ; hash < fromHash ; hash++)
	{
		// Get the cell n before the hash.
		PREFIX(_table_cell) cell = PREFIX(_table_lookup)(table, hash);
		// If this node is a primary entry move it down
		if (MAP_TABLE_HASH_VALUE(cell->value) == hash)
		{
			emptyCell->value = cell->value;
			cell->secondMaps |= (1 << ((fromHash - hash) - 1));
			cell->value = MAP_PLACEHOLDER_VALUE;
			if (hash - toHash < 32)
			{
				return 1;
			}
			return PREFIX(_table_move_gap)(table, hash, toHash, cell);
		}
		int hop = __builtin_ffs(cell->secondMaps);
		if (hop > 0 && (hash + hop) < fromHash)
		{
			PREFIX(_table_cell) hopCell = PREFIX(_table_lookup)(table, hash+hop);
			emptyCell->value = hopCell->value;
			// Update the hop bit for the new offset
			cell->secondMaps |= (1 << ((fromHash - hash) - 1));
			// Clear the hop bit in the original cell
			cell->secondMaps &= ~(1 << (hop - 1));
			hopCell->value = MAP_PLACEHOLDER_VALUE;
			if (hash - toHash < 32)
			{
				return 1;
			}
			return PREFIX(_table_move_gap)(table, hash + hop, toHash, hopCell);
		}
	}
	return 0;
}

/**
 * Rebalancing the table.
 */
static int PREFIX(_table_rebalance)(PREFIX(_table) *table, uint32_t hash)
{
	for (unsigned i = 32; i < table->table_size; i++)
	{
		PREFIX(_table_cell) cell = PREFIX(_table_lookup)(table, hash + i);
		if (MAP_NULL_EQUALITY_FUNCTION(cell->value))
		{
			// We've found a free space, try to move it up.
			return PREFIX(_table_move_gap)(table, hash + i, hash, cell);
		}
	}
	return 0;
}


/**
 * Inserting into the table - the RW lock associated
 * with the table gets locked and the following steps
 * are performed:
 *
 * 1) Hash is computer and the cell corresponding to the
 *     hash is fetched.
 * 2) If the cell is empty, the value is inserted.
 * 3) If the cell is not empty, we try 32-times inserting
 *     to a cell at (hash + i).
 * 4) If this doesn't help and the hash table is nearly full
 *     (80%+), then we resize the table and reinsert.
 * 5) If the table isn't 80%+ full, we try to rebalance and if
 *     that doesn't help either, we force-resize.
 */
static int PREFIX(_insert)(PREFIX(_table) *table,
			   MAP_TABLE_VALUE_TYPE value)
{
	objc_rw_lock_wlock(&table->lock);

	uint32_t hash = MAP_TABLE_HASH_VALUE(value);
	PREFIX(_table_cell) cell = PREFIX(_table_lookup)(table, hash);
	if (MAP_NULL_EQUALITY_FUNCTION(cell->value))
	{
		cell->secondMaps = 0;
		cell->value = value;
		table->table_used++;
		objc_rw_lock_unlock(&table->lock);
		return 1;
	}
	/* If this cell is full, try the next one. */
	for (unsigned int i=0 ; i<32 ; i++)
	{
		PREFIX(_table_cell) second =
		PREFIX(_table_lookup)(table, hash+i);
		if (MAP_NULL_EQUALITY_FUNCTION(second->value))
		{
			cell->secondMaps |= (1 << (i-1));
			second->value = value;
			table->table_used++;
			objc_rw_lock_unlock(&table->lock);
			return 1;
		}
	}
	/* If the table is full, or nearly full, then resize it.  Note that we
	 * resize when the table is at 80% capacity because it's cheaper to copy
	 * everything than spend the next few updates shuffling everything around
	 * to reduce contention.  A hopscotch hash table starts to degrade in
	 * performance at around 90% capacity, so stay below that.
	 */
	if (table->table_used > (0.8 * table->table_size))
	{
		PREFIX(_table_resize)(table);
		objc_rw_lock_unlock(&table->lock);
		return PREFIX(_insert)(table, value);
	}
	/* If this virtual cell is full, rebalance the hash from this point and
	 * try again. */
	if (PREFIX(_table_rebalance)(table, hash))
	{
		objc_rw_lock_unlock(&table->lock);
		return PREFIX(_insert)(table, value);
	}
	/** If rebalancing failed, resize even if we are <80% full.  This can
	 * happen if your hash function sucks.  If you don't want this to happen,
	 * get a better hash function. */
	if (PREFIX(_table_resize)(table))
	{
		objc_rw_lock_unlock(&table->lock);
		return PREFIX(_insert)(table, value);
	}
	
	fprintf(stderr, "Insert failed\n");
	objc_rw_lock_unlock(&table->lock);
	return 0;
}

static void *PREFIX(_table_get_cell)(PREFIX(_table) *table, const void *key)
{
	uint32_t hash = MAP_TABLE_HASH_KEY(key);
	PREFIX(_table_cell) cell = PREFIX(_table_lookup)(table, hash);
	// Value does not exist.
	if (MAP_NULL_EQUALITY_FUNCTION(cell->value))
	{
		if (MAP_TABLE_COMPARE_FUNCTION((MAP_TABLE_KEY_TYPE)key, cell->value))
		{
			return cell;
		}
		uint32_t jump = cell->secondMaps;
		// Look at each offset defined by the jump table to find the displaced location.
		for (int hop = __builtin_ffs(jump) ; hop > 0 ; hop = __builtin_ffs(jump))
		{
			PREFIX(_table_cell) hopCell = PREFIX(_table_lookup)(table, hash+hop);
			if (MAP_TABLE_COMPARE_FUNCTION((MAP_TABLE_KEY_TYPE)key, hopCell->value))
			{
				return hopCell;
			}
			// Clear the most significant bit and try again.
			jump &= ~(1 << (hop-1));
		}
	}
	if (table->old)
	{
		return PREFIX(_table_get_cell)(table->old, key);
	}
	return NULL;
}

__attribute__((unused))
static void PREFIX(_table_move_second)(PREFIX(_table) *table,
				       PREFIX(_table_cell) emptyCell)
{
	uint32_t jump = emptyCell->secondMaps;
	// Look at each offset defined by the jump table to find the displaced location.
	int hop = __builtin_ffs(jump);
	PREFIX(_table_cell) hopCell =
	PREFIX(_table_lookup)(table, MAP_TABLE_HASH_VALUE(emptyCell->value) + hop);
	emptyCell->value = hopCell->value;
	emptyCell->secondMaps &= ~(1 << (hop-1));
	if (0 == hopCell->secondMaps)
	{
		hopCell->value = MAP_PLACEHOLDER_VALUE;
	}
	else
	{
		PREFIX(_table_move_second)(table, hopCell);
	}
}
__attribute__((unused))
static void PREFIX(_remove)(PREFIX(_table) *table, void *key)
{
	objc_rw_lock_wlock(&table->lock);
	PREFIX(_table_cell) cell = PREFIX(_table_get_cell)(table, key);
	if (NULL == cell) { return; }
	// If the cell contains a value, set it to the placeholder and shuffle up
	// everything
	if (0 == cell->secondMaps)
	{
		cell->value = MAP_PLACEHOLDER_VALUE;
	}
	else
	{
		PREFIX(_table_move_second)(table, cell);
	}
	table->table_used--;
	objc_rw_lock_unlock(&table->lock);
}

__attribute__((unused))
static MAP_TABLE_VALUE_TYPE PREFIX(_table_get)(PREFIX(_table) *table,
		   const void *key)
{
	PREFIX(_table_cell) cell = PREFIX(_table_get_cell)(table, key);
	if (NULL == cell)
	{
		return MAP_PLACEHOLDER_VALUE;
	}
	return cell->value;
}
__attribute__((unused))
static void PREFIX(_table_set)(PREFIX(_table) *table, const void *key,
			       MAP_TABLE_VALUE_TYPE value)
{
	PREFIX(_table_cell) cell = PREFIX(_table_get_cell)(table, key);
	if (NULL == cell)
	{
		PREFIX(_insert)(table, value);
	}
	cell->value = value;
}

__attribute__((unused))
static MAP_TABLE_VALUE_TYPE
PREFIX(_next)(PREFIX(_table) *table,
	      struct PREFIX(_table_enumerator) **state)
{
	if (NULL == *state)
	{
		*state = objc_zero_alloc(sizeof(struct PREFIX(_table_enumerator)));
		// Make sure that we are not reallocating the table when we start
		// enumerating
		objc_rw_lock_rlock(&table->lock);
		(*state)->table = table;
		(*state)->index = -1;
		__sync_fetch_and_add(&table->enumerator_count, 1);
		objc_rw_lock_unlock(&table->lock);
	}
	if ((*state)->seen >= (*state)->table->table_used)
	{
		objc_rw_lock_rlock(&table->lock);
		__sync_fetch_and_sub(&table->enumerator_count, 1);
		objc_rw_lock_unlock(&table->lock);
		free(*state);
		return MAP_PLACEHOLDER_VALUE;
	}
	while ((++((*state)->index)) < (*state)->table->table_size)
	{
		if (!MAP_NULL_EQUALITY_FUNCTION(((*state)->table->table[(*state)->index].value)))
		{
			(*state)->seen++;
			return (*state)->table->table[(*state)->index].value;
		}
	}
	// Should not be reached, but may be if the table is unsafely modified.
	objc_rw_lock_rlock(&table->lock);
	table->enumerator_count--;
	objc_rw_lock_unlock(&table->lock);
	free(*state);
	return MAP_PLACEHOLDER_VALUE;
}

/**
 * Returns the current value for an enumerator.  This is used when you remove
 * objects during enumeration.  It may cause others to be shuffled up the
 * table.
 */
__attribute__((unused))
static MAP_TABLE_VALUE_TYPE PREFIX(_current)(PREFIX(_table) *table,
		 struct PREFIX(_table_enumerator) **state)
{
	return (*state)->table->table[(*state)->index].value;
}

#undef PREFIX

#undef MAP_TABLE_NAME
#undef MAP_TABLE_COMPARE_FUNCTION
#undef MAP_TABLE_HASH_KEY
#undef MAP_TABLE_HASH_VALUE

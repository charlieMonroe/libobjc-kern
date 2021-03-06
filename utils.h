/*
 * This file contains functions that are usually in the C stdlib,
 * but aren't that hard to implement here for the sake of speed.
 */

#ifndef OBJC_UTILITIES_H
#define OBJC_UTILITIES_H

/* A private allocator that uses pages for allocations. */
PRIVATE char *objc_string_allocator_alloc(size_t size);

/*
 * Just as the regular strlen function, returns a number of non-zero characters.
 */
static inline unsigned int
objc_strlen(const char *str)
{
	unsigned int counter;
	
	if (str == NULL){
		return 0;
	}
	
	counter = 0;
	while (*str != '\0') {
		++counter;
		++str;
	}
	return counter;
}

/*
 * Unlike the POSIX function, this one handles allocating the new string itself.
 */
static inline char *
objc_strcpy(const char *str)
{
	unsigned int len;
	char *result;
	char *curr_char;
	
	if (str == NULL){
		return NULL;
	}
	
	len = objc_strlen(str);
	result = objc_string_allocator_alloc(len + 1); /* +1 for zero-termination */
	curr_char = result;
	
	while (*str != '\0') {
		*curr_char = *str;
		++curr_char;
		++str;
	}
	
	*curr_char = '\0';
	return result;
}

/*
 * Returns YES if the strings are equal.
 */
static inline BOOL
objc_strings_equal(const char *str1, const char *str2)
{
	unsigned int index;
	
	if (str1 ==  str2){
		return YES;
	}
	
	if (str1 == NULL || str2 == NULL){
		/* Just one of them NULL */
		return NO;
	}
	
	index = 0;
	while (YES) {
		if (str1[index] == str2[index]){
			if (str1[index] == '\0'){
				/* Equal */
				return YES;
			}
			++index;
			continue;
		}
		return NO;
	}
	
	return NO;
}

/*
 * Returns YES if str has prefix pr.
 */
static inline BOOL
objc_string_has_prefix(const char *str, const char *pr)
{
	int prefix_len = objc_strlen(pr);
	for (int i = 0; i < prefix_len; ++i){
		if (pr[i] == '\0'){
			return YES;
		}
		if (str[i] == '\0'){
			return NO;
		}
		if (str[i] != pr[i]){
			return NO;
		}
	}
	return YES;
}

/*
 * Hashes string str.
 */
static inline uint32_t
objc_hash_string(const char *str)
{
	register uint32_t hash = 0;
	register int32_t c;
	while ((c = *str++)){
		hash = c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

/*
 * Returns long long value of the string.
 */
static inline long long
objc_string_long_long_value(const char *str)
{
	long long result = 0;
	char c;
	while (YES){
		c = *str;
		if (c < '0' || c > '9'){
			/* Hit non-alfa char */
			return result;
		}
		
		result *= 10;
		result += (c - '0');
		
		++str;
	}
	return result;
}

/*
 * Returns YES if ptr1 == ptr2;
 */
static inline BOOL
objc_pointers_are_equal(const void *ptr1, const void *ptr2)
{
	return ptr1 == ptr2;
}

/*
 * Hashes a pointer;
 */
static inline unsigned int
objc_hash_pointer(const void *ptr)
{
	/*
	 * Bit-rotate right 4, since the lowest few bits in an object pointer 
	 * will always be 0, which is not so useful for a hash value.
	 */
	return (unsigned int)(((uintptr_t)ptr >> 4) |
	      (uintptr_t)((uintptr_t)ptr << (uintptr_t)((sizeof(id) * 8) - 4)));
}

/*
 * Copies memory from source to destination.
 */
#define objc_copy_memory(dest, src, len) memcpy(dest, src, len)

/*
 * Clears memory by writing zeroes everywhere.
 */
#define objc_memory_zero(mem, size) memset(mem, 0, size)


#endif /* !OBJC_UTILITIES_H */


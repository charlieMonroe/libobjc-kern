/*
  * This file contains functions that are usually in the C stdlib,
  * but aren't that hard to implement here for speed sake.
  */

#ifndef OBJC_UTILITIES_H_
#define OBJC_UTILITIES_H_

#include "types.h" /* For BOOL, NULL, ... */
#include "os.h" /* For objc_alloc */

/*
  * Just as the regular strlen function, returns a number of non-zero characters.
  */
OBJC_INLINE unsigned int objc_strlen(const char *str){
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
OBJC_INLINE char *objc_strcpy(const char *str){
	unsigned int len;
	char *result;
	char *curr_char;
	
	if (str == NULL){
		return NULL;
	}
	
	len = objc_strlen(str);
	result = objc_alloc(len + 1); /* +1 for zero-termination */
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
OBJC_INLINE BOOL objc_strings_equal(const char *str1, const char *str2){
	unsigned int index;
	
	if (str1 == NULL && str2 == NULL){
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
 * Hashes string str.
 */
OBJC_INLINE unsigned int objc_hash_string(const char *str){
	register uint32_t hash = 0;
	register int32_t c;
	while ((c = *str++)){
		hash = c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

/**
 * Returns YES if ptr1 == ptr2;
 */
OBJC_INLINE BOOL objc_pointers_are_equal(const void *ptr1, const void *ptr2){
	return ptr1 == ptr2;
}

/*
 * Hashes a pointer;
 */
OBJC_INLINE unsigned int objc_hash_pointer(const void *ptr){
	// Bit-rotate right 4, since the lowest few bits in an object pointer will
	// always be 0, which is not so useful for a hash value
	return (unsigned int)(((uintptr_t)ptr >> 4) |
			      (uintptr_t)((uintptr_t)ptr << (uintptr_t)((sizeof(id) * 8) - 4)));
}

/*
 * Copies memory from source to destination.
 */
#define objc_copy_memory memcpy

/**
 * Clears memory by writing zeroes everywhere.
 */
OBJC_INLINE void objc_memory_zero(void *mem, unsigned int size){
	char *mem_prt = (char*)mem;
	unsigned int i;
	
	for (i = 0; i < size; ++i){
		mem_prt[i] = 0;
	}
}

#endif /* OBJC_UTILITIES_H_ */


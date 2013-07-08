/*
 * This header ensures compatibility with Apple's and GCC's run-time 
 * implementations. Public functions that do not appear in this run-time,
 * but appear in either of the other ones, are declared here and implemented
 * in the .c file.
 */

#include "types.h"


/**
 * An example porting the sel_registerName function
 * from Apple's run-time.
 */
extern SEL sel_registerName(const char *str);



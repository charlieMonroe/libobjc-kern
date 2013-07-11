/*
 * This file contains declarations of symbols private to the run-time.
 */

#ifndef OBJC_PRIVATE_H_
#define OBJC_PRIVATE_H_

#include "types.h" // For BOOL

extern BOOL objc_runtime_has_been_initialized;

/**
 * Initializes structures necessary for selector registration.
 */
extern void objc_selector_init(void);

/**
 * Inits basic structures for classes.
 */
extern void objc_class_init(void);

/**
 * Registers some basic classes with the run-time.
 */
extern void objc_install_base_classes(void);


#endif /* OBJC_PRIVATE_H_ */

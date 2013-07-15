/*
 * This file contains declarations of symbols private to the run-time.
 */

#ifndef OBJC_PRIVATE_H_
#define OBJC_PRIVATE_H_

#include "types.h" // For BOOL

BOOL objc_runtime_has_been_initialized;

/**
 * Initializes structures necessary for selector registration.
 */
void objc_selector_init(void);

/**
 * Inits basic structures for classes.
 */
void objc_class_init(void);

/**
 * Registers some basic classes with the run-time.
 */
void objc_install_base_classes(void);

void init_dispatch_tables(void);

void objc_arc_init(void);


#endif /* OBJC_PRIVATE_H_ */

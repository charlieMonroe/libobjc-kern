/*
 * This file contains declarations of symbols private to the run-time.
 */

#ifndef OBJC_PRIVATE_H_
#define OBJC_PRIVATE_H_

#include "types.h" // For BOOL

PRIVATE BOOL objc_runtime_has_been_initialized;

/**
 * Initializes structures necessary for selector registration.
 */
PRIVATE void objc_selector_init(void);

/**
 * Inits basic structures for classes.
 */
PRIVATE void objc_class_init(void);

/**
 * Registers some basic classes with the run-time.
 */
PRIVATE void objc_install_base_classes(void);

PRIVATE void init_dispatch_tables(void);

PRIVATE void objc_arc_init(void);

PRIVATE void objc_class_extra_init(void);

PRIVATE void objc_protocol_init(void);


/**
 * Initializes the run-time lock and calls all the above functions.
 */
PRIVATE void objc_runtime_init(void);

#endif /* OBJC_PRIVATE_H_ */

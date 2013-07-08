/*
 * This file contains declarations of symbols private to the run-time.
 */

#ifndef OBJC_PRIVATE_H_
#define OBJC_PRIVATE_H_

#include "runtime.h" /* Needed for objc_runtime_setup_struct. */
#include "class.h" /* Needed for Class. */

extern BOOL objc_runtime_has_been_initialized;

/**
 * Initializes structures necessary for selector registration.
 */
extern void objc_selector_init(void);

/* A pointer to a structure containing all classes */
extern objc_class_holder objc_classes;

/**
 * Inits basic structures for classes.
 */
extern void objc_class_init(void);

/**
 * Registers some basic classes with the run-time.
 */
extern void objc_install_base_classes(void);

/**
 * An external run-time setup structure. This structure shouldn't be modified
 * from anywhere after the run-time is started.
 */
extern objc_runtime_setup_t objc_setup;

#endif /* OBJC_PRIVATE_H_ */

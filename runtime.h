/*
 * This file contains functions and structures that are meant for the run-time
 * setup and initialization.
 */

#ifndef OBJC_RUNTIME_H_
#define OBJC_RUNTIME_H_

/**
 * This function initializes the run-time, checks for any missing function pointers,
 * fills in the built-in function pointers and seals the run-time setup.
 */
extern void objc_runtime_init(void);

#endif /* OBJC_RUNTIME_H_ */

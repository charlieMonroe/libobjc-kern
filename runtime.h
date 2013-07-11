/*
 * This file contains functions and structures that are meant for the run-time
 * setup and initialization.
 */

#ifndef OBJC_RUNTIME_H_
#define OBJC_RUNTIME_H_

#include "os.h"

/**
 * This function initializes the run-time, checks for any missing function pointers,
 * fills in the built-in function pointers and seals the run-time setup.
 */
extern void objc_runtime_init(void);

/**
 * A lock that is used for manipulating with classes - e.g. adding a class
 * to the run-time, etc.
 *
 * All actions performed with this lock locked, should be rarely performed,
 * or at least performed seldomly.
 */
extern objc_rw_lock objc_runtime_lock;

#define OBJC_LOCK_RUNTIME() objc_rw_lock_wlock(&objc_runtime_lock)
#define OBJC_UNLOCK_RUNTIME() objc_rw_lock_unlock(&objc_runtime_lock)

#define OBJC_LOCK_RUNTIME_FOR_SCOPE() OBJC_LOCK_FOR_SCOPE(&objc_runtime_lock)

#endif /* OBJC_RUNTIME_H_ */

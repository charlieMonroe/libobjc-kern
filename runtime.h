/*
 * This file contains functions and structures that are meant for the run-time
 * setup and initialization.
 */

#ifndef OBJC_RUNTIME_H_
#define OBJC_RUNTIME_H_

/*
 * A lock that is used for manipulating with classes - e.g. adding a class
 * to the run-time, etc.
 *
 * All actions performed with this lock locked, should be rarely performed,
 * or at least performed seldomly.
 */
objc_rw_lock objc_runtime_lock;

#define OBJC_LOCK_RUNTIME() OBJC_LOCK(&objc_runtime_lock)
#define OBJC_UNLOCK_RUNTIME() OBJC_UNLOCK(&objc_runtime_lock)

#define OBJC_LOCK_RUNTIME_FOR_SCOPE() OBJC_LOCK_FOR_SCOPE(&objc_runtime_lock)

#endif /* !OBJC_RUNTIME_H_ */

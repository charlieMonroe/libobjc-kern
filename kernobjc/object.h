
#ifndef LIBKERNOBJC_OBJECT_H
#define LIBKERNOBJC_OBJECT_H

/**
 * Returns the class of obj.
 *
 * If obj is an instance of a fake class,
 * the real class is returned.
 */
Class		object_getClass(id obj);

/**
 * Sets the isa pointer of obj and returns the original
 * class.
 */
Class		object_setClass(id obj, Class cls);

/**
 * Returns the name of the class, or NULL if obj == nil.
 */
const char	*object_getClassName(id obj);

/**
 * Sets and gets an ivar.
 */
id		object_getIvar(id obj, Ivar ivar);
void		object_setIvar(id obj, Ivar ivar, id value);

#endif


#ifndef OBJC_MESSAGE_H
#define OBJC_MESSAGE_H

#include "types.h"
#include "selector.h"

PRIVATE Slot objc_class_get_slot(Class cl, SEL selector);

/*
 * TODO - implement in assembly
 */
id objc_msg_send(id receiver, SEL selector, ...);


/**
 * The run-time in a few cases sends a direct ARR
 * messages to objects that do not support ARC,
 * i.e. implement their own ARR methods.
 */
static inline void objc_send_release_msg(id receiver){
	objc_msg_send(receiver, objc_release_selector);
}
static inline id objc_send_retain_msg(id receiver){
	return objc_msg_send(receiver, objc_retain_selector);
}
static inline void objc_send_dealloc_msg(id receiver){
	objc_msg_send(receiver, objc_dealloc_selector);
}
static inline id objc_send_autorelease_msg(id receiver){
	return objc_msg_send(receiver, objc_autorelease_selector);
}

#endif

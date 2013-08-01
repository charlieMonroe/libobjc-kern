
#ifndef OBJC_MESSAGE_H
#define OBJC_MESSAGE_H

/*
 * The run-time in a few cases sends a direct ARR
 * messages to objects that do not support ARC,
 * i.e. those that implement their own ARR methods.
 */

static inline void objc_send_release_msg(id receiver){
	objc_msgSend(receiver, objc_release_selector);
}
static inline id objc_send_retain_msg(id receiver){
	return objc_msgSend(receiver, objc_retain_selector);
}
static inline void objc_send_dealloc_msg(id receiver){
	objc_msgSend(receiver, objc_dealloc_selector);
}
static inline id objc_send_autorelease_msg(id receiver){
	return objc_msgSend(receiver, objc_autorelease_selector);
}
static inline id objc_send_copy_msg(id receiver){
	return objc_msgSend(receiver, objc_copy_selector);
}

#endif

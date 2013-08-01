
#ifndef LIBKERNOBJC_MESSAGE_H
#define LIBKERNOBJC_MESSAGE_H

/*
 * Functions for sending messages to objects. The first argument is the receiver
 * followed by the selector and optional arguments. The only exception is the
 * *_stret variation for sending messages that return structures, which is done
 * by reference, the first argument being the pointer to the structure on the
 * stack.
 */

id		objc_msgSend(id receiver, SEL selector, ...);

void		objc_msgSend_stret(void *ptr, id receiver, SEL selector, ...);



#endif /* !LIBKERNOBJC_MESSAGE_H */

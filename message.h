
#ifndef OBJC_MESSAGE_H
#define OBJC_MESSAGE_H

#include "types.h"

Slot objc_class_get_slot(Class cl, SEL selector);

/*
 * TODO - implement in assembly
 */
extern id objc_msg_send(id receiver, SEL selector, ...);

#endif

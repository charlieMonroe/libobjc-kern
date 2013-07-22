
#ifndef LIBKERNOBJC_MESSAGE_H
#define LIBKERNOBJC_MESSAGE_H

id objc_msgSend(id receiver, SEL selector, ...);
double objc_msgSend_fpret(id receiver, SEL selector, ...);
void objc_msgSend_stret(void *ptr, id receiver, SEL selector, ...);

#endif // LIBKERNOBJC_MESSAGE_H


#ifndef LIBKERNOBJC_IVAR_H
#define LIBKERNOBJC_IVAR_H

const char *ivar_getName(Ivar v);
const char *ivar_getTypeEncoding(Ivar v);
ptrdiff_t ivar_getOffset(Ivar v);

#endif

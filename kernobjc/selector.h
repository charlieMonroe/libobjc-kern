
#ifndef LIBKERNOBJC_SELECTOR_H
#define LIBKERNOBJC_SELECTOR_H

const char	*sel_getName(SEL sel);
const char	*sel_getTypes(SEL sel);
SEL		sel_registerName(const char *str, const char *types);

#endif

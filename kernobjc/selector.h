
#ifndef LIBKERNOBJC_SELECTOR_H
#define LIBKERNOBJC_SELECTOR_H

const char	*sel_getName(SEL sel);
const char	*sel_getTypes(SEL sel);
SEL			sel_registerName(const char *str, const char *types);

/*
 * Since we have no untyped selectors in the run-time, it is hard to make use of
 * @selector(...), since that has no idea about the types.
 *
 * What is done instead, sel_getNamed is used. This doesn't create any selectors,
 * merely looks for an already-registered selector with this name and if it is
 * registered, it is returned. If not, the runtime aborts.
 */
SEL			sel_getNamed(const char *str);

#endif

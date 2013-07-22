
#ifndef LIBKERNOBJC_METHOD_H
#define LIBKERNOBJC_METHOD_H

char		*method_copyArgumentType(Method m, unsigned int index);
char		*method_copyReturnType(Method m);

void		method_exchangeImplementations(Method m1, Method m2);

void		method_getArgumentType(Method m, unsigned int index,
				       char *dst, size_t dst_len);
IMP		method_getImplementation(Method m);
SEL		method_getName(Method m);
unsigned int	method_getNumberOfArguments(Method m);
void		method_getReturnType(Method m, char *dst, size_t dst_len);
const char	*method_getTypeEncoding(Method m);

IMP		method_setImplementation(Method m, IMP imp);

#endif /* !LIBKERNOBJC_METHOD_H */

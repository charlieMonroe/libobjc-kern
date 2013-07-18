
#ifndef LIBKERNOBJC_METHOD_H
#define LIBKERNOBJC_METHOD_H

SEL method_getName(Method m);
IMP method_getImplementation(Method m);
const char *method_getTypeEncoding(Method m);

// TODO
unsigned int method_getNumberOfArguments(Method m);
char *method_copyReturnType(Method m);
char *method_copyArgumentType(Method m, unsigned int index);
void method_getReturnType(Method m, char *dst, size_t dst_len);
void method_getArgumentType(Method m, unsigned int index,
			    char *dst, size_t dst_len);

IMP method_setImplementation(Method m, IMP imp);
void method_exchangeImplementations(Method m1, Method m2);

#endif

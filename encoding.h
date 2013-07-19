#if defined(__clang__) && !defined(__OBJC_RUNTIME_INTERNAL__)
#pragma clang system_header
#endif

#ifndef __LIBOBJC_ENCODING_H_INCLUDED__
#define __LIBOBJC_ENCODING_H_INCLUDED__

const char *objc_skip_type_qualifiers (const char *type);

const char *objc_skip_typespec(const char *type);

const char *objc_skip_argspec(const char *type);


size_t objc_sizeof_type(const char *type);

size_t objc_alignof_type(const char *type);

size_t objc_aligned_size(const char *type);

size_t objc_promoted_size(const char *type);

void method_getReturnType(Method method, char *dst, size_t dst_len);

const char *method_getTypeEncoding(Method method);

void method_getArgumentType(Method method,
                            unsigned int index,
                            char *dst,
                            size_t dst_len);

unsigned method_getNumberOfArguments(Method method);

char * method_copyArgumentType(Method method, unsigned int index);

char * method_copyReturnType(Method method);

#endif // __LIBOBJC_ENCODING_H_INCLUDED__

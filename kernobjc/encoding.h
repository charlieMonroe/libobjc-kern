
#ifndef OBJC_ENCODING_H
#define OBJC_ENCODING_H

#define _C_ID       '@'
#define _C_CLASS    '#'
#define _C_SEL      ':'
#define _C_BOOL     'B'

#define _C_CHR      'c'
#define _C_UCHR     'C'
#define _C_SHT      's'
#define _C_USHT     'S'
#define _C_INT      'i'
#define _C_UINT     'I'
#define _C_LNG      'l'
#define _C_ULNG     'L'
#define _C_LNG_LNG  'q'
#define _C_ULNG_LNG 'Q'

#define _C_FLT      'f'
#define _C_DBL      'd'

#define _C_BFLD     'b'
#define _C_VOID     'v'
#define _C_UNDEF    '?'
#define _C_PTR      '^'

#define _C_CHARPTR  '*'
#define _C_ATOM     '%'

#define _C_ARY_B    '['
#define _C_ARY_E    ']'
#define _C_UNION_B  '('
#define _C_UNION_E  ')'
#define _C_STRUCT_B '{'
#define _C_STRUCT_E '}'
#define _C_VECTOR   '!'

#define _C_COMPLEX  'j'
#define _C_CONST    'r'
#define _C_IN       'n'
#define _C_INOUT    'N'
#define _C_OUT      'o'
#define _C_BYCOPY   'O'
#define _C_ONEWAY   'V'


#define _F_CONST    0x01
#define _F_IN       0x01
#define _F_OUT      0x02
#define _F_INOUT    0x03
#define _F_BYCOPY   0x04
#define _F_ONEWAY   0x08


char		*method_copyArgumentType(Method method, unsigned int index);
char		*method_copyReturnType(Method method);

void		method_getArgumentType(Method method,
								   unsigned int index,
								   char *dst,
								   size_t dst_len);
unsigned	method_getNumberOfArguments(Method method);
void		method_getReturnType(Method method, char *dst, size_t dst_len);
const char	*method_getTypeEncoding(Method method);

size_t		objc_aligned_size(const char *type);
size_t		objc_alignof_type(const char *type);
size_t		objc_promoted_size(const char *type);
size_t		objc_sizeof_type(const char *type);

const char	*objc_skip_argspec(const char *type);
const char	*objc_skip_type_qualifiers (const char *type);
const char	*objc_skip_typespec(const char *type);

#endif

#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "selector.h"
#include "utils.h"
#include "private.h"

#include "kernobjc/encoding.h"

size_t objc_alignof_type (const char *type);

// It would be so nice if this works, but in fact it returns nonsense:
//#define alignof(x) __alignof__(x)
//

#define isdigit(x) (x >= '0' && x <= '9')

#define alignof(type) __builtin_offsetof(struct { const char c; type member; }, member)

const char *objc_skip_type_qualifiers (const char *type)
{
	static const char *type_qualifiers = "rnNoORV";
	while('\0' != *type && strchr(type_qualifiers, *type))
	{
		type++;
	}
	return type;
}

static const char *sizeof_type(const char *type, size_t *size);

const char *objc_skip_typespec(const char *type)
{
	size_t ignored = 0;
	return sizeof_type(type, &ignored);
}

const char *objc_skip_argspec(const char *type)
{
	type = objc_skip_typespec(type);
	while(isdigit(*type)) { type++; }
	return type;
}

PRIVATE size_t lengthOfTypeEncoding(const char *types)
{
	if ((NULL == types) || ('\0' == types[0])) { return 0; }
	const char *end = objc_skip_typespec(types);
	size_t length = end - types;
	return length;
}

static char* copyTypeEncoding(const char *types)
{
	size_t length = lengthOfTypeEncoding(types);
	char *copy = objc_alloc(length + 1, M_ENCODING_TYPE);
	objc_copy_memory(copy, types, length);
	copy[length] = '\0';
	return copy;
}

static const char * findParameterStart(const char *types, unsigned int index)
{
	for (unsigned int i=0 ; i<index ; i++)
	{
		types = objc_skip_argspec(types);
		if ('\0' == *types)
		{
			return NULL;
		}
	}
	return types;
}


typedef const char *(*type_parser)(const char*, void*);

static int parse_array(const char **type, type_parser callback, void *context)
{
	// skip [
	(*type)++;
	int element_count = (int)strtol(*type, (char**)type, 10);
	*type = callback(*type, context);
	// skip ]
	(*type)++;
	return element_count;
}

static void parse_struct_or_union(const char **type, type_parser callback, void *context, char endchar)
{
	// Skip the ( and structure name
	do
	{
		(*type)++;
		// Opaque type has no =definition
		if (endchar == **type) { (*type)++; return; }
	} while('=' != **type);
	// Skip =
	(*type)++;
	
	while (**type != endchar)
	{
		// Structure elements sometimes have their names in front of each
		// element, as in {NSPoint="x"f"y"f} - We need to skip the type name
		// here.
		if ('"'== **type)
		{
			
			do
			{
				(*type)++;
			} while ('"' != **type);
			// Skip the closing "
			(*type)++;
		}
		*type = callback(*type, context);
	}
	// skip }
	(*type)++;
}

static void parse_union(const char **type, type_parser callback, void *context)
{
	parse_struct_or_union(type, callback, context, ')');
}

static void parse_struct(const char **type, type_parser callback, void *context)
{
	parse_struct_or_union(type, callback, context, '}');
}

inline static void round_up(size_t *v, size_t b)
{
	if (0 == b)
	{
		return;
	}
	
	if (*v % b)
	{
		*v += b - (*v % b);
	}
}
inline static size_t objc_max(size_t v, size_t v2)
{
	return v>v2 ? v : v2;
}

static const char *sizeof_union_field(const char *type, size_t *size);

static const char *sizeof_type(const char *type, size_t *size)
{
	type = objc_skip_type_qualifiers(type);
	switch (*type)
	{
			// For all primitive types, we round up the current size to the
			// required alignment of the type, then add the size
#define APPLY_TYPE(typeName, name, capitalizedName, encodingChar) \
case encodingChar:\
{\
round_up(size, (alignof(typeName) * 8));\
*size += (sizeof(typeName) * 8);\
return type + 1;\
}
#define SKIP_ID 1
#define NON_INTEGER_TYPES 1
#include "type_encoding_cases.h"
		case '@':
		{
			round_up(size, (alignof(id) * 8));
			*size += (sizeof(id) * 8);
			if (*(type+1) == '?')
			{
				type++;
			}
			return type + 1;
		}
		case '?':
		case 'v': return type+1;
		case 'j':
		{
			type++;
			switch (*type)
			{
#define APPLY_TYPE(typeName, name, capitalizedName, encodingChar) \
case encodingChar:\
{\
round_up(size, (alignof(_Complex typeName) * 8));\
*size += (sizeof(_Complex typeName) * 8);\
return type + 1;\
}
#include "type_encoding_cases.h"
			}
		}
		case '{':
		{
			const char *t = type;
			parse_struct(&t, (type_parser)sizeof_type, size);
			size_t align = objc_alignof_type(type);
			round_up(size, align * 8);
			return t;
		}
		case '[':
		{
			const char *t = type;
			size_t element_size = 0;
			// FIXME: aligned size
			int element_count = parse_array(&t, (type_parser)sizeof_type, &element_size);
			(*size) += element_size * element_count;
			return t;
		}
		case '(':
		{
			const char *t = type;
			size_t union_size = 0;
			parse_union(&t, (type_parser)sizeof_union_field, &union_size);
			*size += union_size;
			return t;
		}
		case 'b':
		{
			// Consume the b
			type++;
			// Ignore the offset
			strtol(type, (char**)&type, 10);
			// Consume the element type
			type++;
			// Read the number of bits
			*size += strtol(type, (char**)&type, 10);
			return type;
		}
		case '^':
		{
			// All pointers look the same to me.
			*size += sizeof(void*) * 8;
			size_t ignored;
			// Skip the definition of the pointeee type.
			return sizeof_type(type+1, &ignored);
		}
	}
  objc_abort("Invalid type encoding.\n");
	return NULL;
}

static const char *sizeof_union_field(const char *type, size_t *size)
{
	size_t field_size = 0;
	const char *end = sizeof_type(type, &field_size);
	*size = objc_max(*size, field_size);
	return end;
}

static const char *alignof_type(const char *type, size_t *align)
{
	type = objc_skip_type_qualifiers(type);
	switch (*type)
	{
			// For all primitive types, we return the maximum of the new alignment
			// and the old one
#define APPLY_TYPE(typeName, name, capitalizedName, encodingChar) \
case encodingChar:\
{\
*align = objc_max((alignof(typeName) * 8), *align);\
return type + 1;\
}
#define NON_INTEGER_TYPES 1
#define SKIP_ID 1
#include "type_encoding_cases.h"
		case '@':
		{
			*align = objc_max((alignof(id) * 8), *align);\
			if (*(type+1) == '?')
			{
				type++;
			}
			return type + 1;
		}
		case '?':
		case 'v': return type+1;
		case 'j':
		{
			type++;
			switch (*type)
			{
#define APPLY_TYPE(typeName, name, capitalizedName, encodingChar) \
case encodingChar:\
{\
*align = objc_max((alignof(_Complex typeName) * 8), *align);\
return type + 1;\
}
#include "type_encoding_cases.h"
			}
		}
		case '{':
		{
			const char *t = type;
			parse_struct(&t, (type_parser)alignof_type, align);
			return t;
		}
		case '(':
		{
			const char *t = type;
			parse_union(&t, (type_parser)alignof_type, align);
			return t;
		}
		case '[':
		{
			const char *t = type;
			parse_array(&t, (type_parser)alignof_type, &align);
			return t;
		}
		case 'b':
		{
			// Consume the b
			type++;
			// Ignore the offset
			strtol(type, (char**)&type, 10);
			// Alignment of a bitfield is the alignment of the type that
			// contains it
			type = alignof_type(type, align);
			// Ignore the number of bits
			strtol(type, (char**)&type, 10);
			return type;
		}
		case '^':
		{
			*align = objc_max((alignof(void*) * 8), *align);
			// All pointers look the same to me.
			size_t ignored;
			// Skip the definition of the pointeee type.
			return alignof_type(type+1, &ignored);
		}
	}
	objc_abort("Invalid type encoding.\n");
	return NULL;
}

size_t objc_sizeof_type(const char *type)
{
	size_t size = 0;
	sizeof_type(type, &size);
	return size / 8;
}

size_t objc_alignof_type (const char *type)
{
	size_t align = 0;
	alignof_type(type, &align);
	return align / 8;
}

size_t objc_aligned_size(const char *type)
{
	size_t size  = objc_sizeof_type(type);
	size_t align = objc_alignof_type(type);
	return size + (size % align);
}

size_t objc_promoted_size(const char *type)
{
	size_t size = objc_sizeof_type(type);
	return size + (size % sizeof(void*));
}

static inline void _getType(char *dst, const char *types, size_t dst_len){
	size_t length = lengthOfTypeEncoding(types);
	if (length < dst_len){
		objc_copy_memory(dst, types, length);
		dst[length] = '\0';
	}else{
		objc_copy_memory(dst, types, dst_len);
	}
}

void method_getReturnType(Method method, char *dst, size_t dst_len)
{
	if (NULL == method) { return; }
	const char *types = method->selector_types;
	_getType(dst, types, dst_len);
}

void method_getArgumentType(Method method,
                            unsigned int index,
                            char *dst,
                            size_t dst_len)
{
	if (NULL == method) { return; }
	const char *types = findParameterStart(method->selector_types, index);
	if (NULL == types)
	{
		strncpy(dst, "", dst_len);
		return;
	}
	_getType(dst, types, dst_len);
}

unsigned method_getNumberOfArguments(Method method)
{
	if (NULL == method) { return 0; }
	const char *types = method->selector_types;
	unsigned int count = 0;
	while('\0' != *types)
	{
		types = objc_skip_argspec(types);
		count++;
	}
	return count - 1;
}


char* method_copyArgumentType(Method method, unsigned int index)
{
	if (NULL == method) { return NULL; }
	const char *types = findParameterStart(method->selector_types, index);
	if (NULL == types)
	{
		return NULL;
	}
	return copyTypeEncoding(types);
}

char* method_copyReturnType(Method method)
{
	if (NULL == method) { return NULL; }
	return copyTypeEncoding(method->selector_types);
}


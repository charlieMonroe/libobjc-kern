#include "os.h"
#include "kernobjc/types.h"
#include "types.h"
#include "property.h"
#include "class.h"
#include "spinlock.h"
#include "kernobjc/message.h"
#include "selector.h"
#include "message.h"
#include "utils.h"
#include "runtime.h"
#include "kernobjc/arc.h"
#include "kernobjc/class.h"
#include "kernobjc/property.h"
#include "private.h"

PRIVATE int spinlocks[spinlock_count];

static inline BOOL checkAttribute(char field, int attr)
{
	return (field & attr) == attr;
}

static inline id _objc_copy_object(id obj){
	obj = objc_msgSend(obj, objc_copy_selector);
	return obj;
}

/*
 * Public function for getting a property.  
 */
id objc_getProperty(id obj, SEL _cmd, ptrdiff_t offset, BOOL isAtomic)
{
	if (nil == obj) { return nil; }
	char *addr = (char*)obj;
	addr += offset;
	id ret;
	if (isAtomic)
	{
		volatile int *lock = lock_for_pointer(addr);
		lock_spinlock(lock);
		ret = *(id*)addr;
		ret = objc_retain(ret);
		unlock_spinlock(lock);
		ret = objc_autorelease(ret);
	}
	else
	{
		ret = *(id*)addr;
		ret = objc_retainAutorelease(ret);
	}
	return ret;
}

void objc_setProperty(id obj, SEL _cmd, ptrdiff_t offset, id arg, BOOL isAtomic, BOOL isCopy)
{
	if (nil == obj) { return; }
	char *addr = (char*)obj;
	addr += offset;

	if (isCopy)
	{
		arg = _objc_copy_object(arg);
	}
	else
	{
		arg = objc_retain(arg);
	}
	id old;
	if (isAtomic)
	{
		volatile int *lock = lock_for_pointer(addr);
		lock_spinlock(lock);
		old = *(id*)addr;
		*(id*)addr = arg;
		unlock_spinlock(lock);
	}
	else
	{
		old = *(id*)addr;
		*(id*)addr = arg;
	}
	objc_release(old);
}

void objc_setProperty_atomic(id obj, SEL _cmd, id arg, ptrdiff_t offset)
{
	char *addr = (char*)obj;
	addr += offset;
	arg = objc_retain(arg);
	volatile int *lock = lock_for_pointer(addr);
	lock_spinlock(lock);
	id old = *(id*)addr;
	*(id*)addr = arg;
	unlock_spinlock(lock);
	objc_release(old);
}

void objc_setProperty_atomic_copy(id obj, SEL _cmd, id arg, ptrdiff_t offset)
{
	char *addr = (char*)obj;
	addr += offset;

	arg = _objc_copy_object(arg);
	volatile int *lock = lock_for_pointer(addr);
	lock_spinlock(lock);
	id old = *(id*)addr;
	*(id*)addr = arg;
	unlock_spinlock(lock);
	objc_release(old);
}

void objc_setProperty_nonatomic(id obj, SEL _cmd, id arg, ptrdiff_t offset)
{
	char *addr = (char*)obj;
	addr += offset;
	arg = objc_retain(arg);
	id old = *(id*)addr;
	*(id*)addr = arg;
	objc_release(old);
}

void objc_setProperty_nonatomic_copy(id obj, SEL _cmd, id arg, ptrdiff_t offset)
{
	char *addr = (char*)obj;
	addr += offset;
	id old = *(id*)addr;
	*(id*)addr = _objc_copy_object(arg);
	objc_release(old);
}


/*
 * Structure copy function.  This is provided for compatibility with the Apple
 * APIs (it's an ABI function, so it's semi-public), but it's a bad design so
 * it's not used.  The problem is that it does not identify which of the
 * pointers corresponds to the object, which causes some excessive locking to
 * be needed.
 */
void objc_copyPropertyStruct(void *dest,
                             void *src,
                             ptrdiff_t size,
                             BOOL atomic,
                             BOOL strong)
{
	if (atomic)
	{
		volatile int *lock = lock_for_pointer(src < dest ? src : dest);
		volatile int *lock2 = lock_for_pointer(src < dest ? dest : src);
		lock_spinlock(lock);
		lock_spinlock(lock2);
		memcpy(dest, src, size);
		unlock_spinlock(lock);
		unlock_spinlock(lock2);
	}
	else
	{
		memcpy(dest, src, size);
	}
}

/*
 * Get property structure function.  Copies a structure from an ivar to another
 * variable.  Locks on the address of src.
 */
void objc_getPropertyStruct(void *dest,
                            void *src,
                            ptrdiff_t size,
                            BOOL atomic,
                            BOOL strong)
{
	if (atomic)
	{
		volatile int *lock = lock_for_pointer(src);
		lock_spinlock(lock);
		memcpy(dest, src, size);
		unlock_spinlock(lock);
	}
	else
	{
		memcpy(dest, src, size);
	}
}

/*
 * Set property structure function.  Copes a structure to an ivar.  Locks on
 * dest.
 */
void objc_setPropertyStruct(void *dest,
                            void *src,
                            ptrdiff_t size,
                            BOOL atomic,
                            BOOL strong)
{
	if (atomic)
	{
		volatile int *lock = lock_for_pointer(dest);
		lock_spinlock(lock);
		memcpy(dest, src, size);
		unlock_spinlock(lock);
	}
	else
	{
		memcpy(dest, src, size);
	}
}


Property class_getProperty(Class cls, const char *name)
{
	objc_property_list *properties = cls->properties;
	while (NULL != properties)
	{
		for (int i=0 ; i<properties->size ; i++)
		{
			Property p = &properties->list[i];
			if (objc_strings_equal(property_getName(p), name))
			{
				return p;
			}
		}
		properties = properties->next;
	}
	return NULL;
}

Property *class_copyPropertyList(Class cls, unsigned int *outCount)
{
	if (cls == Nil || cls->properties == NULL || cls->properties->size == 0){
		objc_debug_log("Returning NULL property list since the class [%p;%s%s] has"
					   " either no properties (%p) or the list is empty (%d)\n",
					   cls, class_getName(cls), cls->flags.meta ? "[meta]" : "",
					   cls->properties,
					   cls->properties == NULL ? 0 : cls->properties->size);
		*outCount = 0;
		return NULL;
	}
	return objc_property_list_copy_list(cls->properties, outCount);
}
static const char* property_getIVar(Property property) {
	const char *iVar = property_getAttributes(property);
	if (iVar != 0)
	{
		while ((*iVar != 0) && (*iVar != 'V'))
		{
			iVar++;
		}
		if (*iVar == 'V')
		{
			return iVar+1;
		}
	}
	return 0;
}

const char *property_getName(Property property)
{
	if (NULL == property) { return NULL; }

	const char *name = property->name;
	if (NULL == name) { return NULL; }
	if (name[0] == 0)
	{
		name += name[1];
	}
	return name;
}

// PRIVATE size_t lengthOfTypeEncoding(const char *types);

/*
 * The compiler stores the type encoding of the getter.  We replace this with
 * the type encoding of the property itself.  We use a 0 byte at the start to
 * indicate that the swap has taken place.
 */
static const char *property_getTypeEncoding(Property property)
{
	if (NULL == property) { return NULL; }
	if (NULL == property->getter_types) { return NULL; }

	const char *name = property->getter_types;
	if (name[0] == 0)
	{
		return &name[1];
	}
	size_t typeSize = lengthOfTypeEncoding(name);
	char *buffer = objc_alloc(typeSize + 2, M_PROPERTY_TYPE);
	buffer[0] = 0;
	objc_copy_memory(buffer+1, name, typeSize);
	buffer[typeSize+1] = 0;
	if (!__sync_bool_compare_and_swap(&(property->getter_types), name, buffer))
	{
		objc_dealloc(buffer, M_PROPERTY_TYPE);
	}
	return &property->getter_types[1];
}

PRIVATE const char *constructPropertyAttributes(Property property,
                                                const char *iVarName)
{
	const char *name = (char*)property->name;
	const char *typeEncoding = property_getTypeEncoding(property);
	size_t typeSize = (NULL == typeEncoding) ? 0 : strlen(typeEncoding);
	size_t nameSize = (NULL == name) ? 0 : strlen(name);
	size_t iVarNameSize = (NULL == iVarName) ? 0 : strlen(iVarName);
	// Encoding is T{type},V{name}, so 4 bytes for the "T,V" that we always
	// need.  We also need two bytes for the leading null and the length.
	size_t encodingSize = typeSize + nameSize + 6;
	char flags[20];
	size_t i = 0;
	// Flags that are a comma then a character
	if (checkAttribute(property->attributes, OBJC_PR_readonly))
	{
		flags[i++] = ',';
		flags[i++] = 'R';
	}
	if (checkAttribute(property->attributes, OBJC_PR_retain))
	{
		flags[i++] = ',';
		flags[i++] = '&';
	}
	if (checkAttribute(property->attributes, OBJC_PR_copy))
	{
		flags[i++] = ',';
		flags[i++] = 'C';
	}
	if (checkAttribute(property->attributes2, OBJC_PR_weak))
	{
		flags[i++] = ',';
		flags[i++] = 'W';
	}
	if (checkAttribute(property->attributes2, OBJC_PR_dynamic))
	{
		flags[i++] = ',';
		flags[i++] = 'D';
	}
	if ((property->attributes & OBJC_PR_nonatomic) == OBJC_PR_nonatomic)
	{
		flags[i++] = ',';
		flags[i++] = 'N';
	}
	encodingSize += i;
	flags[i] = '\0';
	size_t setterLength = 0;
	size_t getterLength = 0;
	if ((property->attributes & OBJC_PR_getter) == OBJC_PR_getter)
	{
		getterLength = strlen(property->getter_name);
		encodingSize += 2 + getterLength;
	}
	if ((property->attributes & OBJC_PR_setter) == OBJC_PR_setter)
	{
		setterLength = strlen(property->setter_name);
		encodingSize += 2 + setterLength;
	}
	if (NULL != iVarName)
	{
		encodingSize += 2 + iVarNameSize;
	}
	unsigned char *encoding = objc_alloc(encodingSize, M_PROPERTY_TYPE);
	// Set the leading 0 and the offset of the name
	unsigned char *insert = encoding;
	BOOL needsComma = NO;
	*(insert++) = 0;
	*(insert++) = 0;
	// Set the type encoding
	if (NULL != typeEncoding)
	{
		*(insert++) = 'T';
		memcpy(insert, typeEncoding, typeSize);
		insert += typeSize;
		needsComma = YES;
	}
	// Set the flags
	memcpy(insert, flags, i);
	insert += i;
	if ((property->attributes & OBJC_PR_getter) == OBJC_PR_getter)
	{
		if (needsComma)
		{
			*(insert++) = ',';
		}
		i++;
		needsComma = YES;
		*(insert++) = 'G';
		memcpy(insert, property->getter_name, getterLength);
		insert += getterLength;
	}
	if ((property->attributes & OBJC_PR_setter) == OBJC_PR_setter)
	{
		if (needsComma)
		{
			*(insert++) = ',';
		}
		i++;
		needsComma = YES;
		*(insert++) = 'S';
		memcpy(insert, property->setter_name, setterLength);
		insert += setterLength;
	}
	// If the instance variable name is the same as the property name, then we
	// use the same string for both, otherwise we write the ivar name in the
	// attributes string and then a null and then the name.
	if (NULL != iVarName)
	{
		if (needsComma)
		{
			*(insert++) = ',';
		}
		*(insert++) = 'V';
		memcpy(insert, iVarName, iVarNameSize);
		insert += iVarNameSize;
	}
	*(insert++) = '\0';
	encoding[1] = (unsigned char)(uintptr_t)(insert - encoding);
	memcpy(insert, name, nameSize);
	insert += nameSize;
	*insert = '\0';
	// If another thread installed the encoding string while we were computing
	// it, then discard the one that we created and return theirs.
	if (!__sync_bool_compare_and_swap(&(property->name), name, (char*)encoding))
	{
		objc_dealloc(encoding, M_PROPERTY_TYPE);
		return property->name + 2;
	}
	return (const char*)(encoding + 2);
}


const char *property_getAttributes(Property property)
{
	if (NULL == property) { return NULL; }

	const char *name = (char*)property->name;
	if (name[0] == 0)
	{
		return name + 2;
	}
	return constructPropertyAttributes(property, NULL);
}


objc_property_attribute_t *property_copyAttributeList(Property property,
                                                      unsigned int *outCount)
{
	if (NULL == property) { return NULL; }
	objc_property_attribute_t attrs[12];
	int count = 0;

	const char *types = property_getTypeEncoding(property);
	if (NULL != types)
	{
		attrs[count].name = "T";
		attrs[count].value = types;
		count++;
	}
	if (checkAttribute(property->attributes, OBJC_PR_readonly))
	{
		attrs[count].name = "R";
		attrs[count].value = "";
		count++;
	}
	if (checkAttribute(property->attributes, OBJC_PR_copy))
	{
		attrs[count].name = "C";
		attrs[count].value = "";
		count++;
	}
	if (checkAttribute(property->attributes, OBJC_PR_retain) ||
	    checkAttribute(property->attributes2, OBJC_PR_strong))
	{
		attrs[count].name = "&";
		attrs[count].value = "";
		count++;
	}
	if (checkAttribute(property->attributes2, OBJC_PR_dynamic) &&
	    !checkAttribute(property->attributes2, OBJC_PR_synthesized))
	{
		attrs[count].name = "D";
		attrs[count].value = "";
		count++;
	}
	if (checkAttribute(property->attributes2, OBJC_PR_weak))
	{
		attrs[count].name = "W";
		attrs[count].value = "";
		count++;
	}
	if ((property->attributes & OBJC_PR_nonatomic) == OBJC_PR_nonatomic)
	{
		attrs[count].name = "N";
		attrs[count].value = "";
		count++;
	}
	if ((property->attributes & OBJC_PR_getter) == OBJC_PR_getter)
	{
		attrs[count].name = "G";
		attrs[count].value = property->getter_name;
		count++;
	}
	if ((property->attributes & OBJC_PR_setter) == OBJC_PR_setter)
	{
		attrs[count].name = "S";
		attrs[count].value = property->setter_name;
		count++;
	}
	const char *name = property_getIVar(property);
	if (name != NULL)
	{
		attrs[count].name = "V";
		attrs[count].value = name;
		count++;
	}

	objc_property_attribute_t *propAttrs = objc_zero_alloc(sizeof(objc_property_attribute_t) * count, M_PROPERTY_TYPE);
	memcpy(propAttrs, attrs, count * sizeof(objc_property_attribute_t));
	if (NULL != outCount)
	{
		*outCount = count;
	}
	return propAttrs;
}

PRIVATE struct objc_property propertyFromAttrs(const objc_property_attribute_t *attributes,
                                               unsigned int attributeCount,
                                               const char **name)
{
	struct objc_property p = { 0 };
	for (unsigned int i=0 ; i<attributeCount ; i++)
	{
		switch (attributes[i].name[0])
		{
			case 'T':
			{
				size_t typeSize = strlen(attributes[i].value);
				char *buffer = objc_alloc(typeSize + 2, M_PROPERTY_TYPE);
				buffer[0] = 0;
				memcpy(buffer+1, attributes[i].value, typeSize);
				buffer[typeSize+1] = 0;
				p.getter_types = buffer;
				break;
			}
			case 'S':
			{
				p.setter_name = objc_strcpy(attributes[i].value);
				break;
			}
			case 'G':
			{
				p.getter_name = objc_strcpy(attributes[i].value);
				break;
			}
			case 'V':
			{
				*name = attributes[i].value;
				break;
			}
			case 'C':
			{
				p.attributes |= OBJC_PR_copy;
				break;
			}
			case 'R':
			{
				p.attributes |= OBJC_PR_readonly;
				break;
			}
			case 'W':
			{
				p.attributes2 |= OBJC_PR_weak;
				break;
			}
			case '&':
			{
				p.attributes |= OBJC_PR_retain;
				break;
			}
			case 'N':
			{
				p.attributes |= OBJC_PR_nonatomic;
				break;
			}
			case 'D':
			{
				p.attributes2 |= OBJC_PR_dynamic;
				break;
			}
		}
	}
	return p;
}

BOOL class_addProperty(Class cls,
                       const char *name,
                       const objc_property_attribute_t *attributes, 
                       unsigned int attributeCount)
{
	if ((Nil == cls) || (NULL == name) || (class_getProperty(cls, name) != 0)) { return NO; }
	const char *iVarname = NULL;
	struct objc_property p = propertyFromAttrs(attributes, attributeCount, &iVarname);
	// If the iVar name is not the same as the name, then we need to construct
	// the attributes string now, otherwise we can construct it lazily.
	p.name = name;
	constructPropertyAttributes(&p, iVarname);

	objc_property_list *l = objc_property_list_create(1);
	objc_copy_memory(&l->list, &p, sizeof(struct objc_property));
	
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	l->next = cls->properties;
	cls->properties = l;
	return YES;
}

void class_replaceProperty(Class cls,
                           const char *name,
                           const objc_property_attribute_t *attributes,
                           unsigned int attributeCount)
{
	if ((Nil == cls) || (NULL == name)) { return; }
	Property old = class_getProperty(cls, name);
	if (NULL == old)
	{
		class_addProperty(cls, name, attributes, attributeCount);
		return;
	}
	const char *iVarname = 0;
	struct objc_property p = propertyFromAttrs(attributes, attributeCount, &iVarname);
	p.name = name;
	OBJC_LOCK_RUNTIME_FOR_SCOPE();
	constructPropertyAttributes(&p, iVarname);
	objc_copy_memory(old, &p, sizeof(struct objc_property));
}
char *property_copyAttributeValue(Property property,
                                  const char *attributeName)
{
	if ((NULL == property) || (NULL == attributeName)) { return NULL; }
	switch (attributeName[0])
	{
		case 'T':
		{
			const char *types = property_getTypeEncoding(property);
			return (NULL == types) ? NULL : objc_strcpy(types);
		}
		case 'D':
		{
			return checkAttribute(property->attributes2, OBJC_PR_dynamic) &&
			       !checkAttribute(property->attributes2, OBJC_PR_synthesized) ? objc_strcpy("") : 0;
		}
		case 'V':
		{
			return objc_strcpy(property_getIVar(property));
		}
		case 'S':
		{
			return objc_strcpy(property->setter_name);
		}
		case 'G':
		{
			return objc_strcpy(property->getter_name);
		}
		case 'R':
		{
			return checkAttribute(property->attributes, OBJC_PR_readonly) ? objc_strcpy("") : 0;
		}
		case 'W':
		{
			return checkAttribute(property->attributes2, OBJC_PR_weak) ? objc_strcpy("") : 0;
		}
		case 'C':
		{
			return checkAttribute(property->attributes, OBJC_PR_copy) ? objc_strcpy("") : 0;
		}
		case '&':
		{
			return checkAttribute(property->attributes, OBJC_PR_retain) ||
			       checkAttribute(property->attributes2, OBJC_PR_strong) ? objc_strcpy("") : 0;
		}
		case 'N':
		{
			return checkAttribute(property->attributes, OBJC_PR_nonatomic) ? objc_strcpy("") : 0;
		}
	}
	return 0;
}

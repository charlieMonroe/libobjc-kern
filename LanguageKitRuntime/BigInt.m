#import "BigInt.h"
#import "BlockClosure.h"

#ifdef _KERNEL
	#include <sys/ctype.h>

	#define isalnum(c) (isalpha(c) || isdigit(c))
#else
	#include <ctype.h>
#endif

Class BigIntClass;

static BigInt *BigIntYES;
static BigInt *BigIntNO;

@implementation BigInt
+ (void) initialize
{
	if ([BigInt class] == self)
	{
		BigIntClass = self;
		BigIntNO = [[BigInt bigIntWithLongLong: 0] retain];
		BigIntYES = [[BigInt bigIntWithLongLong: 1] retain];
	}
}
+ (BigInt*) bigIntWithCString:(const char*) aString
{
	BigInt *b = [[[BigInt alloc] init] autorelease];
	b->_data.signedIntegralValue = [[NSString stringWithUTF8String:aString] longLongValue];
	return b;
}
// Pretend that big ints are long longs
- (const char*)objCType
{
	return "q";
}
+ (BigInt*) bigIntWithLongLong:(long long)aVal
{
	BigInt *b = [[[BigInt alloc] init] autorelease];

	b->_data.signedIntegralValue = aVal;

	return b;
}
+ (BigInt*) bigIntWithLong:(long)aVal
{
	BigInt *b = [[[BigInt alloc] init] autorelease];
	b->_data.signedIntegralValue = aVal;
	return b;
}
+ (BigInt*) bigIntWithUnsignedLong:(unsigned long)aVal
{
	BigInt *b = [[[BigInt alloc] init] autorelease];
	b->_data.unsignedIntegralValue = aVal;
	return b;
}

+ (BigInt*) bigIntWithMP:(mpz_t)aVal
{
	BigInt *b = [[[BigInt alloc] init] autorelease];
	b->_data.unsignedIntegralValue = aVal;
	return b;
}

id objc_autoreleaseReturnValue(id);

static inline id LKObjCAutoreleaseReturnValue(id object)
{
#if __has_feature(objc_arc)
	return objc_autoreleaseReturnValue(object);
#else

	return [object autorelease];
#endif
}

#define op2(name, operation) \
- (LKObject) name:(id)other\
{\
	if (nil == other)\
	{\
		[NSException raise: @"BigIntException"\
		            format: @"nil argument to " #name];\
	}\
	BigInt *b = [[BigInt alloc] init];\
	b->_data.unsignedIntegralValue = _data.unsignedIntegralValue operation (((BigInt*)other)->_data.unsignedIntegralValue);\
	return LKObjectFromObject(LKObjCAutoreleaseReturnValue(b));\
}

#define op(name) op2(name, name)

op2(plus, +)
op2(sub, -)
op2(mul, *)
op2(mod, %)
op2(div, /)

#define op_cmp(name, func) \
  - (LKObject) name: (id)other \
{\
  	if (nil == other)\
	{\
		[NSException raise: @"BigIntException"\
		            format: @"nil argument to min"];\
	}\
	LKObject returnVal;\
	\
	 if (_data.unsignedIntegralValue func (((BigInt*)other)->_data.unsignedIntegralValue))	\
		 returnVal = LKObjectFromObject(self);\
	 else\
		 returnVal = LKObjectFromObject(other);\
	return returnVal;\
}\

op_cmp(min, <=)
op_cmp(max, >=) 
- (id)and: (id)a
{
	return _data.unsignedIntegralValue && [a intValue] ? BigIntYES : BigIntNO;
}
- (id)or: (id)a;
{
	return _data.unsignedIntegralValue || [a intValue] ? BigIntYES : BigIntNO;
}
op2(bitwiseAnd, &);
op2(bitwiseOr, |);
- (id)not
{
	if (_data.unsignedIntegralValue != 0)
	{
		return BigIntNO;
	}
	return BigIntYES;
}
#define CTYPE(name, op) \
- (BOOL)name\
{\
	int  max = (int)_data.unsignedIntegralValue;\
	return 0 != op(max);\
}

CTYPE(isAlphanumeric, isalnum)
CTYPE(isUppercase, isupper)
CTYPE(isLowercase, islower)
CTYPE(isDigit, isdigit)
CTYPE(isAlphabetic, isalpha)
CTYPE(isWhitespace, isspace)
- (id)value
{
	return self;
}

static inline int __cmp(unsigned long long l1, unsigned long long l2){
	if (l1 == l2){
		return 0;
	}
	if (l1 < l2){
		return -1;
	}
	return 1;
}
#define CMP(sel, op) \
- (BOOL) sel:(id)other \
{\
	if ([other isKindOfClass: BigIntClass])\
	{\
		BigInt *o = other;\
		return __cmp(_data.unsignedIntegralValue, o->_data.unsignedIntegralValue) op 0;\
	}\
	if ([other respondsToSelector:@selector(intValue)])\
	{\
		return __cmp(_data.unsignedIntegralValue, [other intValue]) op 0;\
	}\
	return NO;\
}
CMP(isLessThan, <)
CMP(isGreaterThan, >)
CMP(isLessThanOrEqualTo, <=)
CMP(isGreaterThanOrEqualTo, >=)
CMP(isEqual, ==)
- (id) timesRepeat:(id) aBlock
{
	id result = nil;
	for (unsigned long long i = 0 ; i < _data.unsignedIntegralValue ; i++)
	{
		result = [aBlock value];
	}
	return result;
}
- (id) to: (id) other by: (id) incr do: (id) aBlock
{
	id result = nil;
	if ([other isKindOfClass: object_getClass(self)] && [incr isKindOfClass: object_getClass(self)]){
		unsigned long long i = _data.unsignedIntegralValue;
		unsigned long long max = ((BigInt*)other)->_data.unsignedIntegralValue;
		unsigned long long inc = ((BigInt*)incr)->_data.unsignedIntegralValue;
		for (;i<=max;i+=inc)
		{
			result = [(BlockClosure*)aBlock value:
					  [BigInt bigIntWithLongLong: (long long) i]];
		}
	}
	else
	{
		unsigned long long i, max, inc;
		i = _data.unsignedIntegralValue;

		if ([other isKindOfClass: object_getClass(self)])
		{
			max = ((BigInt*)other)->_data.unsignedIntegralValue;
		}
		else if ([other respondsToSelector: @selector(intValue)])
		{
			max = [other intValue];
		}
		else if ([other respondsToSelector: @selector(longValue)])
		{
			max = [other longValue];
		}
		else
		{
			return nil;
		}

		if ([incr isKindOfClass: object_getClass(self)])
		{
			inc = ((BigInt*)incr)->_data.unsignedIntegralValue;
		}
		else if ([incr respondsToSelector: @selector(intValue)])
		{
			inc = [incr intValue];
		}
		else if ([incr respondsToSelector: @selector(longValue)])
		{
			inc = [incr longValue];
		}
		else
		{
			return nil;
		}
		
		for (;i<=max;i+=inc)
		{
			result = [(BlockClosure*)aBlock value:
					  [BigInt bigIntWithLongLong: (long long) i]];
		}
	}
	return result;
}
- (id) to: (id) other do: (id) aBlock
{
	return [self to: other by: [BigInt bigIntWithLongLong: 1] do: aBlock];
}

- (NSString*)descriptionWithLocale:(id)locale
{
	NSString *str = [NSString stringWithFormat:@"%llu", _data.unsignedIntegralValue];
	return str;
}
- (NSString*)description
{
	return [self descriptionWithLocale: nil];
}
- (NSString*) stringValue
{
	return [self descriptionWithLocale: nil];
}

#define CASTMETHOD(returnType, name, gmpFunction)\
- (returnType) name {\
	return (returnType) _data.unsignedIntegralValue;\
}

CASTMETHOD(signed char, charValue, mpz_get_si)
CASTMETHOD(unsigned char, unsignedCharValue, mpz_get_ui)
CASTMETHOD(short int, shortValue, mpz_get_si)
CASTMETHOD(unsigned short int, unsignedShortValue, mpz_get_ui)
CASTMETHOD(int, intValue, mpz_get_si)
CASTMETHOD(unsigned int, unsignedIntValue, mpz_get_ui)
CASTMETHOD(long int, longValue, mpz_get_si)
CASTMETHOD(unsigned long int, unsignedLongValue, mpz_get_ui)
//FIXME: GMP doesn't have a function to get a long long int, so these methods aren't too useful.
CASTMETHOD(long long int, longLongValue, mpz_get_si)
CASTMETHOD(unsigned long long int, unsignedLongLongValue, mpz_get_ui)
CASTMETHOD(BOOL, boolValue, mpz_get_ui)

- (id)copyWithZone: (NSZone*)aZone
{
	BigInt *new = [object_getClass(self) allocWithZone: aZone];
	new->_data.unsignedIntegralValue = _data.unsignedIntegralValue;
	return new;
}
-(id)copy{
	return [self copyWithZone:nil];
}

@end

@implementation NSNumber (LanguageKit)
- (id)ifTrue: (id(^)(void))block
{
	if ([self boolValue])
	{
		return block();
	}
	return 0;
}
- (id)ifFalse: (id(^)(void))block
{
	if (![self boolValue])
	{
		return block();
	}
	return 0;
}
- (id)ifTrue: (id(^)(void))block ifFalse: (id(^)(void))falseBlock
{
	if ([self boolValue])
	{
		return block();
	}
	return falseBlock();
}
- (id)timesRepeat: (id)block
{
	void *ret = NULL;
	unsigned long long max = [self unsignedLongLongValue];
	for (unsigned long long i=0 ; i<max ; i++)
	{
		ret = ((id(^)(void))block)();
	}
	return ret;
}
- (id)to: (id)to by: (id)by do: (id)block
{
	unsigned long long i = [self unsignedLongLongValue];
	id result = nil;
	for (unsigned long long step = [by unsignedLongLongValue]; i<step ; i++)
	{
		result = ((id(^)(id))block)([NSNumber numberWithUnsignedLongLong: i]);
	}
	return result;
}
- (id)to: (id)to do: (id)block
{
	return [self to: to by: [NSNumber numberWithInt: 1] do: block];
}
@end

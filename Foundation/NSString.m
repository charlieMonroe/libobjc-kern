

#import "NSString.h"
#import "NSArray.h"
#import "NSException.h"
#import "../kernobjc/runtime.h"
#import "../utils.h"

#ifdef _KERNEL
	#include <machine/stdarg.h>
#else
	#include <stdarg.h>
#endif

NSString *const NSStringOutOfBoundsException = @"NSStringOutOfBoundsException";
NSString *const NSStringAbstractClassAllocationException = @"NSStringAbstractClassAllocationException";

static inline void NSStringRaiseOutOfBoundsException(void){
	[[NSException exceptionWithName:NSStringOutOfBoundsException reason:@"" userInfo:nil] raise];
}

MALLOC_DEFINE(M_NSSTRING_TYPE, "NSString", "NSString backing");

/* This is the subclass that is used for allocations. */
@interface _NSString : NSString {
	BOOL _dontFreeOnDealloc;
}

@end

@implementation _NSString

-(void)dealloc{
	if (!_dontFreeOnDealloc){
		objc_dealloc(_data.mutable, M_NSSTRING_TYPE);
	}
	
	[super dealloc];
}

-(id)init{
	if ((self = [super init]) != nil){
		_length = 0;
		_dontFreeOnDealloc = YES;
		_data.immutable = "";
	}
	return self;
}
-(id)initWithBytesNoCopy:(void *)bytes length:(NSUInteger)length
				encoding:(NSStringEncoding)encoding freeWhenDone:(BOOL)flag{
	if ((self = [super init]) != nil){
		_data.mutable = bytes;
		_length = (unsigned int)length;
		_dontFreeOnDealloc = !flag;
	}
	return self;
}

@end



@implementation NSString

+(id)alloc{
	id obj = [super alloc];
	object_setClass(obj, [_NSString class]);
	return obj;
}
+(void)load{
	if (self == [NSString class]){
		class_addMethodsFromClass([_KKConstString class], [NSString class]);
	}
}
+(id)string{
	return @"";
}
+(id)stringWithCharacters:(const unichar*)chars length:(NSUInteger)length{
	return [[[_NSString alloc] initWithCString:chars length:length] autorelease];
}
+(id)stringWithCString:(const char*)byteString length:(NSUInteger)length{
	return [[[_NSString alloc] initWithCString:byteString length:length] autorelease];
}
+(id)stringWithCString:(const char*)byteString{
	return [[[_NSString alloc] initWithCString:byteString
								   length:objc_strlen(byteString)] autorelease];
}
+(id)stringWithString:(NSString*)string{
	return [[[_NSString alloc] initWithString:string] autorelease];
}
+(id)stringWithFormat:(NSString*)format, ...{
	va_list ap;
	va_start(ap, format);
	NSString *str = [[_NSString alloc] initWithFormat:format arguments: ap];
	va_end(ap);
	return [str autorelease];
}
+(id)stringWithUTF8String:(const unichar*)str{
	return [self stringWithCString:str];
}

-(id)copy{
	return [self retain];
}

-(id)init{
	if ((self = [super init]) != nil){
		_length = 0;
		_data.immutable = "";
	}
	return self;
}
-(id)initWithCString:(const char*)byteString length:(NSUInteger)length{
	if ((self = [super init]) != nil){
		_data.mutable = objc_alloc(length + 1, M_NSSTRING_TYPE);
		memcpy(_data.mutable, byteString, length);
		_data.mutable[length] = '\0';
		
		_length = (unsigned int)length;
	}
	return self;
}

-(id)initWithCString:(const char*)byteString{
	return [self initWithCString:byteString length:objc_strlen(byteString)];
}
-(id)initWithBytesNoCopy:(void *)bytes length:(NSUInteger)length
				encoding:(NSStringEncoding)encoding freeWhenDone:(BOOL)flag{
	if ((self = [super init]) != nil){
		_data.mutable = bytes;
		_length = (unsigned int)length;
	}
	return self;
}
-(id)initWithString:(NSString*)string{
	return [self initWithCString:string->_data.immutable];
}
-(id)initWithFormat:(NSString*)format, ...{
	va_list ap;
	va_start(ap, format);
	self = [self initWithFormat:format arguments: ap];
	va_end(ap);
	
	return self;
}
-(id)initWithFormat:(NSString*)format arguments:(va_list)argList{
	return [[[[[NSMutableString alloc] initWithFormat:format arguments:argList] autorelease] copy] autorelease];
}

-(BOOL)isMemberOfClass:(Class)cls{
	return cls == [NSString class];
}
-(BOOL)isKindOfClass:(Class)cls{
	return cls == [NSString class] || [super isKindOfClass:cls];
}
-(NSUInteger)length{
	return _length;
}

-(unichar)characterAtIndex:(NSUInteger)index{
	if (index >= _length){
		NSStringRaiseOutOfBoundsException();
	}
	
	return _data.immutable[index];
}
-(void)getCharacters:(unichar*)buffer{
	memcpy(buffer, _data.immutable, _length + 1);
}
-(void)getCharacters: (unichar*)buffer range:(NSRange)aRange{
	/* TODO - zero termination ? */
	memcpy(buffer, _data.immutable + aRange.location, aRange.length);
}

-(NSString*)stringByAppendingString:(NSString*)aString{
	NSUInteger totalLength = _length + aString->_length;
	char *buffer = objc_alloc(totalLength + 1, M_NSSTRING_TYPE);
	memcpy(buffer, _data.immutable, _length);
	memcpy(buffer + _length, aString->_data.immutable, aString->_length);
	buffer[totalLength] = '\0';
	
	return [[NSString alloc] initWithBytesNoCopy:buffer length:totalLength
										encoding:0 freeWhenDone:YES];
}

-(NSArray*)componentsSeparatedByString:(NSString*)separator{
	NSMutableArray *array = [NSMutableArray array];
	
	NSUInteger separatorLen = separator->_length;
	const char *separatorStr = separator->_data.immutable;
	
	NSUInteger start = 0;
	for (NSUInteger i = 0; i < _length;){
		
		if (i + separatorLen >= _length){
			break;
		}
		
		BOOL fail = NO;
		for (NSUInteger o = 0; o < separatorLen; ++o){
			if (_data.immutable[i + o] != separatorStr[o]){
				fail = YES;
				break;
			}
		}
		
		if (fail){
			++i;
			continue;
		}
		
		/* Found the component. */
		[array addObject:[self substringInRange:NSMakeRange(start, i - start)]];
		i += separatorLen;
		start = i;
	}
	
	if (start == 0){
		[array addObject:self];
	}else if (start < _length){
		[array addObject:[self substringInRange:NSMakeRange(start, _length - start)]];
	}
	
	return array;
}
-(NSString*)substringFromIndex:(NSUInteger)index{
	return [self substringInRange:NSMakeRange(index, _length - index)];
}
-(NSString*)substringToIndex:(NSUInteger)index{
	return [self substringInRange:NSMakeRange(0, index)];
}
-(NSString*)substringInRange:(NSRange)range{
	char *buffer = objc_alloc(range.length + 1, M_NSSTRING_TYPE);
	memcpy(buffer, _data.immutable + range.location, range.length);
	buffer[range.length] = '\0';
	return [[[NSString alloc] initWithBytesNoCopy:buffer length:range.length
										encoding:0 freeWhenDone:YES] autorelease];
}

-(id)mutableCopy{
	return [[NSMutableString alloc] initWithString:self];
}

-(BOOL)hasPrefix:(NSString*)aString{
	if ([aString length] > _length){
		return NO;
	}
	if ([aString length] == _length){
		return [self isEqualToString:aString];
	}
	
	for (NSUInteger i = 0; i < aString->_length; ++i){
		if (_data.immutable[i] != aString->_data.immutable[i]){
			return NO;
		}
	}
	return YES;
	
}
-(BOOL)hasSuffix:(NSString*)aString{
	if ([aString length] > _length){
		return NO;
	}
	if ([aString length] == _length){
		return [self isEqualToString:aString];
	}
	
	for (NSUInteger i = _length - aString->_length; i < _length; ++i){
		if (_data.immutable[i] != aString->_data.immutable[i]){
			return NO;
		}
	}
	return YES;
}
-(BOOL)isEqual:(id)anObject{
	if ([anObject isKindOfClass:[NSString class]]){
		return [self isEqualToString:anObject];
	}
	return NO;
}
-(BOOL)isEqualToString:(NSString*)aString{
	if (self == aString){
		return YES;
	}
	return objc_strings_equal(_data.immutable, aString->_data.immutable);
}

-(NSUInteger)hash{
	return objc_hash_string(_data.immutable);
}

-(const char*)cString{
	return _data.immutable;
}
-(const unichar*)UTF8String{
	return _data.immutable;
}

-(int)intValue{
	return (int)objc_string_long_long_value(_data.immutable);
}
-(long)longValue{
	return (long)objc_string_long_long_value(_data.immutable);
}
-(long long)longLongValue{
	return objc_string_long_long_value(_data.immutable);
}


-(NSRange)rangeOfString:(NSString*)str{
	return [self rangeOfString:str inRange:NSMakeRange(0, _length)];
}
-(NSRange)rangeOfString:(NSString*)str inRange:(NSRange)range{
	NSUInteger strLen = str->_length;
	const char *strStr = str->_data.immutable;
	
	for (NSUInteger i = range.location; i < NSMaxRange(range);){
		if (i + strLen >= _length){
			break;
		}
		
		BOOL fail = NO;
		for (NSUInteger o = 0; o < strLen; ++o){
			if (_data.immutable[i + o] != strStr[o]){
				fail = YES;
				break;
			}
		}
		
		if (fail){
			++i;
			continue;
		}
		
		return NSMakeRange(i, strLen);
	}
	return NSMakeRange(NSNotFound, 0);
}

-(NSString*)stringByReplacingOccurrencesOfString:(NSString*)needle withString:(NSString*)str{
	NSMutableString *result = [[NSMutableString alloc] initWithString:self];
	[result replaceOccurrencesOfString:needle withString:str options:0 range:NSMakeRange(0, _length)];
	return [result autorelease];
}

@end

@implementation NSMutableString

+(id)alloc{
	id obj = [super alloc];
	object_setClass(obj, [NSMutableString class]);
	return obj;
}

-(void)appendCString:(const unichar *)str length:(NSUInteger)length{
	NSUInteger totalLen = _length + length;
	_data.mutable = objc_realloc(_data.mutable, totalLen + 1, M_NSSTRING_TYPE);
	memcpy(_data.mutable + _length, str, length);
	_data.mutable[totalLen] = '\0';
}
-(void)appendString:(NSString*)string{
	objc_assert(string != nil, "Appending nil string!\n");
	
	[self appendCString:string->_data.immutable length:string->_length];
}
-(void)appendFormat:(NSString*)format, ...{
	va_list ap;
	va_start(ap, format);
	NSString *str = [[NSString alloc] initWithFormat:format arguments: ap];
	va_end(ap);
	[self appendString:str];
}

-(id)copy{
	return [[NSString alloc] initWithString:self];
}
-(void)dealloc{
	objc_dealloc(_data.mutable, M_NSSTRING_TYPE);
	
	[super dealloc];
}
-(id)initWithFormat:(NSString *)format arguments:(va_list)argList{
	if ((self = [super init]) != nil){
		_data.mutable = objc_zero_alloc(1, M_NSSTRING_TYPE);
		_data.mutable[0] = '\0';
		
		unichar	fbuf[1024];
		unichar	*fmt = fbuf;
		size_t	len;
		
		/*
		 * First we provide an array of unichar characters containing the
		 * format string.  For performance reasons we try to use an on-stack
		 * buffer if the format string is small enough ... it almost always
		 * will be.
		 */
		len = [format length];
		if (len >= 1024)
		{
			fmt = objc_alloc((len+1)*sizeof(unichar), M_NSSTRING_TYPE);
		}
		[format getCharacters: fmt];
		fmt[len] = '\0';
		
		extern void GSPrivateFormat(NSMutableString *s, const unichar *format, va_list ap);
		GSPrivateFormat(self, fmt, argList);
		if (fmt != fbuf)
		{
			objc_dealloc(fmt, M_NSSTRING_TYPE);
		}
	}
	return self;
}

-(void)replaceCharactersInRange:(NSRange)range withString:(NSString*)str{
	if (NSMaxRange(range) > _length){
		NSStringRaiseOutOfBoundsException();
	}
	
	NSUInteger stringLength = [str length];
	if (stringLength == range.length){
		/* We're lucky, just replace the chars */
		for (NSUInteger i = range.location; i < NSMaxRange(range); ++i){
			_data.mutable[i] = str->_data.immutable[i];
		}
		return;
	}
	
	/* We need to do some shifting. */
	if (stringLength < range.length){
		/* We need to shift the end of the string more to the front. */
		NSUInteger shiftBy = range.length - stringLength;
		for (NSUInteger i = NSMaxRange(range); i < _length; ++i){
			_data.mutable[i - shiftBy] = _data.immutable[i];
		}
		
		_data.mutable = objc_realloc(_data.mutable, _length - shiftBy + 1, M_NSSTRING_TYPE);
		_length -= shiftBy;
		_data.mutable[_length] = '\0';
	}else{
		/* Extend and shift towards the end */
		/* We need to shift the end of the string more to the front. */
		NSUInteger shiftBy = [str length] - range.length;
		_data.mutable = objc_realloc(_data.mutable, _length + shiftBy + 1, M_NSSTRING_TYPE);
		for (NSUInteger i = _length - 1; i >= range.location + stringLength; --i){
			_data.mutable[i] = _data.immutable[i - shiftBy];
		}
		
		_length += shiftBy;
		_data.mutable[_length] = '\0';
	}
	
	for (NSUInteger i = range.location; i < stringLength; ++i){
		_data.mutable[i] = str->_data.immutable[i];
	}
}
-(void)replaceOccurrencesOfString:(NSString *)needle withString:(id)str options:(NSUInteger)options range:(NSRange)range{
	objc_assert(options == 0, "No support for any searching options!\n");
	
	NSRange r;
	while (YES){
		r = [self rangeOfString:needle inRange:range];
		if (r.location == NSNotFound){
			return;
		}
		
		[self replaceCharactersInRange:r withString:str];
	}
}

@end



#import "NSString.h"
#import "NSArray.h"
#import "NSException.h"
#import "../utils.h"

#ifdef _KERNEL
	#include <machine/stdarg.h>
#else
	#include <stdarg.h>
#endif

NSString *const NSStringOutOfBoundsException = @"NSStringOutOfBoundsException";

static inline void NSStringRaiseOutOfBoundsException(void){
	[[NSException exceptionWithName:NSStringOutOfBoundsException reason:@"" userInfo:nil] raise];
}

MALLOC_DEFINE(M_NSSTRING_TYPE, "NSString", "NSString backing");

@implementation NSString

+(id)string{
	return @"";
}
+(id)stringWithCharacters:(const unichar*)chars length:(NSUInteger)length{
	return [[[self alloc] initWithCString:chars length:length] autorelease];
}
+(id)stringWithCString:(const char*)byteString length:(NSUInteger)length{
	return [[[self alloc] initWithCString:byteString length:length] autorelease];
}
+(id)stringWithCString:(const char*)byteString{
	return [[[self alloc] initWithCString:byteString
								   length:objc_strlen(byteString)] autorelease];
}
+(id)stringWithString:(NSString*)string{
	return [[[self alloc] initWithString:string] autorelease];
}
+(id)stringWithFormat:(NSString*)format, ...{
	va_list ap;
	va_start(ap, format);
	NSString *str = [[NSString alloc] initWithFormat:format arguments: ap];
	va_end(ap);
	return [str autorelease];
}
+(id)stringWithUTF8String:(const unichar*)str{
	return [self stringWithCString:str];
}


-(id)copy{
	return [self retain];
}
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
-(id)initWithCString:(const char*)byteString length:(NSUInteger)length{
	if ((self = [super init]) != nil){
		_data.mutable = objc_alloc(length + 1, M_NSSTRING_TYPE);
		memcpy(_data.mutable, byteString, length);
		_data.mutable[length] = '\0';
		
		_length = length;
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
		_length = length;
		_dontFreeOnDealloc = !flag;
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
	// TODO
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
	return atoi(_data.immutable);
}

@end

@implementation NSMutableString

-(void)appendString:(NSString*)string{
	objc_assert(string != nil, "Appending nil string!\n");
	
	NSUInteger strLen = string->_length;
	NSUInteger totalLen = _length + strLen;
	_data.mutable = objc_realloc(_data.mutable, totalLen + 1, M_NSSTRING_TYPE);
	memcpy(_data.mutable + _length, string->_data.immutable, strLen);
	_data.mutable[totalLen] = '\0';
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

@end


#import "NSObject.h"
#import "NSTypes.h"

#ifdef _KERNEL
	#include <machine/stdarg.h>
#else
	#include <stdarg.h>
#endif

@class NSArray;

/* The encoding typedef and some types are for compatibility reasons only. */
typedef NSUInteger NSStringEncoding;

#define NSUTF8StringEncoding ((NSStringEncoding)0)

@interface NSString : NSObject {
	union {
		const char	*immutable;
		char		*mutable;
	} _data;
	unsigned int _length;
}


+(id)string;
+(id)stringWithCharacters:(const unichar*)chars length:(NSUInteger)length;
+(id)stringWithCString:(const char*)byteString length:(NSUInteger)length;
+(id)stringWithCString:(const char*)byteString;
+(id)stringWithFormat:(NSString*)format,...;
+(id)stringWithString:(NSString*)string;
+(id)stringWithUTF8String:(const unichar*)str;

-(id)init;
-(id)initWithCString:(const char*)byteString;
-(id)initWithCString:(const char*)byteString length:(NSUInteger)length;

-(id)initWithBytesNoCopy:(void*)bytes length:(NSUInteger)length
				  encoding:(NSStringEncoding)encoding
			  freeWhenDone:(BOOL)flag;

-(id)initWithString:(NSString*)string;
-(id)initWithFormat:(NSString*)format, ...;
-(id)initWithFormat:(NSString*)format arguments:(va_list)argList;

-(NSUInteger)length;

-(unichar)characterAtIndex:(NSUInteger)index;
-(void)getCharacters:(unichar*)buffer;
-(void)getCharacters: (unichar*)buffer range:(NSRange)aRange;

-(NSString*)stringByAppendingString:(NSString*)aString;
-(NSString*)stringByReplacingOccurrencesOfString:(NSString*)needle withString:(NSString*)str;

-(id)mutableCopy;

-(NSArray*)componentsSeparatedByString:(NSString*)separator;
-(NSString*)substringFromIndex:(NSUInteger)index;
-(NSString*)substringToIndex:(NSUInteger)index;
-(NSString*)substringInRange:(NSRange)range;

-(BOOL)hasPrefix:(NSString*)aString;
-(BOOL)hasSuffix:(NSString*)aString;
-(BOOL)isEqual:(id)anObject;
-(BOOL)isEqualToString:(NSString*)aString;

-(NSRange)rangeOfString:(NSString*)str;
-(NSRange)rangeOfString:(NSString*)str inRange:(NSRange)range;

-(NSUInteger)hash;

-(const char*)cString;
-(const unichar*)UTF8String;

-(int)intValue;
-(long)longValue;
-(long long)longLongValue;

@end


@interface NSMutableString : NSString

-(void)appendCString:(const unichar*)str length:(NSUInteger)length;
-(void)appendString:(NSString*)string;
-(void)appendFormat:(NSString*)format, ...;

-(void)replaceCharactersInRange:(NSRange)range withString:(NSString*)str;
-(void)replaceOccurrencesOfString:(NSString*)needle withString:str options:(NSUInteger)options range:(NSRange)range;

@end


extern NSString *const NSStringOutOfBoundsException;

#define NSLog(format...) \
		objc_log("%s\n", [[NSString stringWithFormat:format] UTF8String])


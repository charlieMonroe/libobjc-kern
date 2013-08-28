
#import "NSObject.h"
#import "NSTypes.h"

@class NSArray;

typedef NSUInteger NSStringEncoding;

@interface NSString : NSObject {
	union {
		const char	*immutable;
		char		*mutable;
	} _data;
	NSUInteger _length;
	BOOL _dontFreeOnDealloc;
}


+(id)string;
+(id)stringWithCharacters:(const unichar*)chars length:(NSUInteger)length;
+(id)stringWithCString:(const char*)byteString length:(NSUInteger)length;
+(id)stringWithCString:(const char*)byteString;
+(id)stringWithFormat:(NSString*)format,...;
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

-(NSArray*)componentsSeparatedByString:(NSString*)separator;
-(NSString*)substringFromIndex:(NSUInteger)index;
-(NSString*)substringToIndex:(NSUInteger)index;
-(NSString*)substringInRange:(NSRange)range;

-(BOOL)hasPrefix:(NSString*)aString;
-(BOOL)hasSuffix:(NSString*)aString;
-(BOOL)isEqual:(id)anObject;
-(BOOL)isEqualToString:(NSString*)aString;

-(NSUInteger)hash;

-(const char*)cString;
-(const unichar*)UTF8String;

-(int)intValue;


@end


@interface NSMutableString : NSString

@end


extern NSString *const NSStringOutOfBoundsException;

#define NSLog(format...) \
		objc_log("%s\n", [[NSString stringWithFormat:format] UTF8String])

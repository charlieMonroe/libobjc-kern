
#import "NSObject.h"

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


+ (id) string;
+ (id) stringWithCharacters: (const unichar*)chars length: (NSUInteger)length;
+ (id) stringWithCString: (const char*)byteString encoding: (NSStringEncoding)encoding;
+ (id) stringWithCString: (const char*)byteString length: (NSUInteger)length;
+ (id) stringWithCString: (const char*)byteString;
+ (id) stringWithFormat: (NSString*)format,...;
+ (id) stringWithContentsOfFile:(NSString *)path;

// Initializing Newly Allocated Strings
- (id) init;
- (id) initWithCString: (const char*)byteString length: (NSUInteger)length;

- (id) initWithCString: (const char*)byteString;
- (id) initWithString: (NSString*)string;
- (id) initWithFormat: (NSString*)format, ...;
- (id) initWithFormat: (NSString*)format arguments: (va_list)argList;

// Getting a String's Length
- (NSUInteger) length;

// Accessing Characters
- (unichar) characterAtIndex: (NSUInteger)index;
- (void) getCharacters: (unichar*)buffer;
- (void) getCharacters: (unichar*)buffer range: (NSRange)aRange;

- (NSString*) stringByAppendingString: (NSString*)aString;

// Dividing Strings into Substrings
- (NSArray*) componentsSeparatedByString: (NSString*)separator;
- (NSString*) substringFromIndex: (NSUInteger)index;
- (NSString*) substringToIndex: (NSUInteger)index;

- (BOOL) hasPrefix: (NSString*)aString;
- (BOOL) hasSuffix: (NSString*)aString;
- (BOOL) isEqual: (id)anObject;
- (BOOL) isEqualToString: (NSString*)aString;

- (NSUInteger) hash;

// Getting C Strings
- (const char*) cString;

- (int) intValue;


@end

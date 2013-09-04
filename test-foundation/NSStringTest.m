
#import "../Foundation/Foundation.h"
#import "../utils.h"

void run_string_test(void);
void run_string_test(void){
	@autoreleasepool {
		[NSString string]; // Force initialize
		
		
		NSString *constString = @"Hello World";
		const char *cString = [constString UTF8String];
		objc_assert(objc_strings_equal(cString, "Hello World"), "Wrong value\n");
		
		NSString *str = [NSString string];
		objc_assert([str length] == 0, "Wrong value\n");
		objc_assert([str isEqualToString:@""], "Wrong value\n");
		
		str = [NSString stringWithCString:"Hello"];
		objc_assert([str isEqualToString:@"Hello"], "Not equal\n");
		
		str = [NSString stringWithFormat:@"Hello %i %s %@!", 123, "world",
			  [[[KKObject alloc] init] autorelease]];
		objc_assert([str isEqualToString:@"Hello 123 world KKObject<>!"], "Not equal\n");
		
		str = [@"Hello" stringByAppendingString:@" world"];
		objc_assert([str isEqualToString:@"Hello world"], "Not equal\n");
		objc_assert([str hasPrefix:@"Hello"], "Not equal\n");
		objc_assert([str hasSuffix:@"Hello world"], "Not equal\n");
		
		str = [@"Hello" stringByReplacingOccurrencesOfString:@"e" withString:@"o"];
		objc_assert([str isEqualToString:@"Hollo"], "Not equal\n");
		
		NSMutableString *mutableStr = [[str mutableCopy] autorelease];
		objc_assert([mutableStr isEqualToString:str], "Not equal\n");
		
		[mutableStr appendString:@" hollo"];
		objc_assert([mutableStr isEqualToString:@"Hollo hollo"], "Not equal\n");
		
		[mutableStr appendFormat:@" %@", @"hollo"];
		objc_assert([mutableStr isEqualToString:@"Hollo hollo hollo"], "Not equal\n");
		
		[mutableStr replaceCharactersInRange:[mutableStr rangeOfString:@" hollo"] withString:@""];
		objc_assert([mutableStr isEqualToString:@"Hollo hollo"], "Not equal\n");
		
		[mutableStr replaceCharactersInRange:[mutableStr rangeOfString:@" hollo"] withString:@""];
		objc_assert([mutableStr isEqualToString:@"Hollo"], "Not equal\n");
		
		[mutableStr replaceOccurrencesOfString:@"ll" withString:@"" options:0 range:NSMakeRange(0, [mutableStr length])];
		objc_assert([mutableStr isEqualToString:@"Hoo"], "Not equal\n");
		
		[mutableStr replaceOccurrencesOfString:@"Hoo" withString:@"Hollllo" options:0 range:NSMakeRange(0, [mutableStr length])];
		objc_assert([mutableStr isEqualToString:@"Hollllo"], "Not equal\n");
	}
}

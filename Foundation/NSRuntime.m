#import "Foundation.h"
#import "../kernobjc/runtime.h"

Class NSClassFromString(NSString *className){
	return objc_getClass([className UTF8String]);
}

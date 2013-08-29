#import "Foundation.h"
#import "../kernobjc/runtime.h"
#import "../kernobjc/encoding.h"

#ifdef _KERNEL
	#include <sys/ctype.h>
#else
	#include <ctype.h>
#endif

Class NSClassFromString(NSString *className){
	return objc_getClass([className UTF8String]);
}

NSString *NSStringFromSelector(SEL select){
	return [NSString stringWithUTF8String:sel_getName(select)];
}

const char *NSGetSizeAndAlignment(const char *typePtr, NSUInteger *sizep, NSUInteger *alignp)
{
	if (typePtr != NULL)
    {
		/* Skip any offset, but don't call objc_skip_offset() as that's buggy.
		 */
		if (*typePtr == '+' || *typePtr == '-')
		{
			typePtr++;
		}
		while (isdigit(*typePtr))
		{
			typePtr++;
		}
		typePtr = objc_skip_type_qualifiers (typePtr);
		if (sizep)
		{
			*sizep = objc_sizeof_type (typePtr);
		}
		if (alignp)
		{
			*alignp = objc_alignof_type (typePtr);
		}
		typePtr = objc_skip_typespec (typePtr);
    }
	return typePtr;
}

SEL NSSelectorFromString(NSString *selName){
	return sel_getNamed([selName UTF8String]);
}

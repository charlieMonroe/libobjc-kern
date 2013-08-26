
#include "kernobjc/types.h"

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(objc_root_class)
__attribute__((objc_root_class))
#endif
@interface KKObject {
	id isa;
	int retain_count;
}

+(id)alloc;
+(id)new;

+(Class)class;
+(BOOL)respondsToSelector:(SEL)selector;

-(Class)class;
-(id)init;

-(BOOL)respondsToSelector:(SEL)selector;

-(void)dealloc;
-(id)retain;
-(void)release;

@end


@interface _KKConstString : KKObject {
	const char *_cString;
	unsigned int _length;
}

-(const char*)cString;
-(unsigned int)length;

@end


/**
 * Definition of the Protocol type.  Protocols are objects, but are rarely used
 * as such.
 */
@interface Protocol : KKObject
{
@public
	OBJC_PROTOCOL_FIELDS;
}

@end







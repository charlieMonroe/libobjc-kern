
#include "types.h"

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(objc_root_class)
__attribute__((objc_root_class))
#endif
@interface KKObject {
	id isa;
	int __retain_count;
}

+(id)alloc;
+(id)new;

+(Class)class;
+(BOOL)respondsToSelector:(SEL)selector;

+(void)initialize;
+(void)load;


-(BOOL)isMemberOfClass:(Class)cls;
-(BOOL)isKindOfClass:(Class)cls;
-(Class)class;


-(id)init;

-(unsigned long long)hash;
-(BOOL)isEqual:(id)otherObj;

-(BOOL)respondsToSelector:(SEL)selector;

-(id)autorelease;
-(id)copy;
-(void)dealloc;
-(id)retain;
-(void)release;

@end


@interface _KKConstString : KKObject {
	const char *_cString;
	unsigned int _length;
}

-(const char*)cString;
-(unsigned long long)length;

@end


/* 
 * When a kernel module is unloaded, the memory containing the code gets unloaded
 * as well. To prevent page faults in kernel, the runtime replaces all IMPs
 * from that particular module with an IMP that throws this exception.
 */
@interface __KKUnloadedModuleException : KKObject

+(void)raiseUnloadedModuleException:(id)obj selector:(SEL)selector;

@property (readwrite, retain) id object;
@property (readwrite, assign) SEL selector;

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







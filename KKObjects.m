#include "kernobjc/runtime.h"
#include "os.h"
#include "private.h"
#include "types.h"
#include "init.h"

#import "KKObjects.h"

#pragma mark KKObject

@implementation KKObject

+(id)alloc
{
	return class_createInstance((Class)self, 0);
}
+(id)new
{
	return [[self alloc] init];
}

+(Class)class
{
	return self;
}
+(BOOL)respondsToSelector:(SEL)selector
{
	return class_respondsToSelector(self->isa, selector);
}
+(void)load
{
	/* Must fix up the blocks. */
	objc_blocks_init();
}
+(void)initialize
{
	// No-op right now
}


+(void)dealloc
{
	// No-op
}

+(id)autorelease{
	return self;
}
+(id)retain
{
	return self;
}

+(void)release
{
	// No-op
}

-(id)autorelease{
	return objc_autorelease(self);
}
-(void)dealloc
{
	object_dispose((id)self);
}

-(Class)class
{
	return object_getClass(self);
}
-(id)init
{
	return self;
}

-(BOOL)isEqual:(id)otherObj{
	return self == otherObj;
}

-(void)release
{
	objc_debug_log("Releasing object %p[%i]\n", self, self->retain_count);
	int retain_cnt = __sync_fetch_and_sub(&self->retain_count, 1);
	if (retain_cnt == 0){
		/* Dealloc */
		[self dealloc];
	}else if (retain_cnt < 0){
		objc_abort("Over-releasing an object %p (%i)!", self, retain_cnt);
	}
}

-(id)retain
{
	objc_debug_log("Retaining an object %p\n", self);
	__sync_add_and_fetch(&self->retain_count, 1);
	return self;
}

-(BOOL)respondsToSelector:(SEL)selector
{
	return class_respondsToSelector([self class], selector);
}


@end

#pragma mark -
#pragma mark _KKConstString

@implementation _KKConstString

-(const char*)cString
{
	return self->_cString;
}

-(void)dealloc
{
	// No-op
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
}
#pragma clang diagnostic pop

-(unsigned int)length
{
	return self->_length;
}

-(void)release
{
	// No-op
}

-(id)retain
{
	return self;
}

@end


@implementation Protocol
- (BOOL)conformsTo:(Protocol*)p
{
	return protocol_conformsToProtocol(self, p);
}
- (id)retain
{
	return self;
}
- (void)release {}
+ (Class)class { return self; }
- (id)self { return self; }
@end



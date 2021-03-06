#include "kernobjc/runtime.h"
#include "os.h"
#include "private.h"
#include "types.h"
#include "init.h"
#include "utils.h"

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
	if (self == [KKObject class]){
		/*
		 * This is a relatively ugly hack that prevents the -autorelease message
		 * from becoming recursive.
		 */
		objc_debug_log("Updating %s's custom ARR flag to NO\n",
					   class_getName(self));
		self->flags.has_custom_arr = NO;
	}
}


+(void)dealloc
{
	// No-op
}

+(id)autorelease
{
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
+(id)copy
{
	return self;
}

-(id)autorelease
{
	return objc_autorelease(self);
}
-(void)dealloc
{
	object_dispose((id)self);
}

-(id)description{
	return @"KKObject<>";
}

-(Class)class
{
	return object_getClass(self);
}
-(BOOL)isMemberOfClass:(Class)cls{
	return [self class] == cls;
}
-(BOOL)isKindOfClass:(Class)cls{
	Class self_cl = [self class];
	while (self_cl != Nil) {
		if (self_cl == cls){
			return YES;
		}
		self_cl = class_getSuperclass(self_cl);
	}
	return NO;
}

-(id)init
{
	return self;
}

-(unsigned long long)hash{
	return objc_hash_pointer(self);
}
-(BOOL)isEqual:(id)otherObj{
	return self == otherObj;
}

-(id)copy
{
	return object_copy(self, class_getInstanceSize([self class]));
}
-(void)release
{
	objc_debug_log("Releasing object %p[%i]\n", self, self->__retain_count);
	int retain_cnt = __sync_fetch_and_sub(&self->__retain_count, 1);
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
	__sync_add_and_fetch(&self->__retain_count, 1);
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
-(id)copy
{
	return [self retain];
}
-(void)dealloc
{
	// No-op
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
}
#pragma clang diagnostic pop

-(unsigned long long)hash{
	return objc_hash_string(self->_cString);
}

-(unsigned long long)length
{
	return self->_length;
}

-(id)autorelease
{
	return self;
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

@implementation __KKUnloadedModuleException

+(void)raiseUnloadedModuleException:(id)obj selector:(SEL)selector{
	__KKUnloadedModuleException *exc = [[[self alloc] init] autorelease];
	[exc setObject:obj];
	[exc setSelector:selector];
	@throw exc;
}

-(void)dealloc{
	[self setObject:nil];
	
	[super dealloc];
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



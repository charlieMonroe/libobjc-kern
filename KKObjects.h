
#include "kernobjc/types.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-root-class"
@interface KKObject {
#pragma clang diagnostic pop
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




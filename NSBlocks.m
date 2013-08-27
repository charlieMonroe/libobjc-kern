#import "KKObjects.h"

#define KKDeclareAndImplementBlockClass(class, superclass)				\
@interface class : superclass																		\
@end																														\
@implementation class																						\
@end


KKDeclareAndImplementBlockClass(_NSBlock, KKObject)
KKDeclareAndImplementBlockClass(_NSConcreteStackBlock, _NSBlock)
KKDeclareAndImplementBlockClass(_NSConcreteGlobalBlock, _NSBlock)
KKDeclareAndImplementBlockClass(_NSConcreteMallocBlock, _NSBlock)



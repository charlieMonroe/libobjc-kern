
#ifndef OBJC_FOUNDATION_H
#define OBJC_FOUNDATION_H

#import "NSArray.h"
#import "NSBundle.h"
#import "NSDictionary.h"
#import "NSException.h"
#import "NSIndexSet.h"
#import "NSNull.h"
#import "NSObject.h"
#import "NSSet.h"
#import "NSString.h"
#import "NSThread.h"
#import "NSTypes.h"
#import "NSValue.h"

extern Class NSClassFromString(NSString *class);


/* Macros from the GNUstep foundation. */

#ifndef SUPERINIT
#define SUPERINIT							\
	if ((self = [super init]) == nil){		\
		return nil;							\
	}
#endif

#ifndef SELFINIT
#define SELFINIT							\
	if ((self = [self init]) == nil){		\
		return nil;							\
	}
#endif

#ifndef	ASSIGN
#define	ASSIGN(object,value)	({\
  id __object = object; \
  object = [(value) retain]; \
  [__object release]; \
})
#endif

#ifndef	AUTORELEASE
#define	AUTORELEASE(object)	[(object) autorelease]
#endif

#ifndef RELEASE
#define RELEASE(obj) [obj release]
#endif

#ifndef FOREACH
	#define FOREACH(arr, var, type) for (type var in arr)
#endif

#ifndef D
	#define D(objs...) [NSDictionary dictionaryWithObjectsAndKeys:objs]
#endif

#define NSCParameterAssert(condition)			\
	objc_assert((condition), "Invalid parameter not satisfying: %s\n", #condition)

#endif

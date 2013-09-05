CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0
CFLAGS	+= -fblocks
CFLAGS	+= -I../

LDFLAGS += --print-gc-sections

KMOD	= objc_foundation

SRCS	= Foundation.c \
		GSFormat.m	\
		NSArray.m	\
		NSBundle.m	\
		NSDictionary.m	\
		NSEnumerator.m	\
		NSException.m	\
		NSIndexSet.m	\
		NSMethodSignature.m	\
		NSNull.m	\
		NSNumber.m	\
		NSObject.m	\
		NSRuntime.m	\
		NSSet.m	\
		NSString.m	\
		NSThread.m	\
		NSValue.m	

.include <bsd.kmod.mk>


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0
CFLAGS	+= -fblocks
CFLAGS	+= -I../

KMOD	= smalltalk_runtime

SRCS	= smalltalk.c \
			BlockClosure.m \
			NSArray+map.m \
			NSObject+log.m \
			NSObject+yourself.m \
			NSString+casting.m \
			NSString+comma.m \
			NSValue+structs.m

.include <bsd.kmod.mk>


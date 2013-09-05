CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0
CFLAGS	+= -fblocks
CFLAGS	+= -I../

KMOD	= objc_langkit

SRCS	= LanguageKitRuntime.c \
			BigInt.m \
			BlockClosure.m \
			BlockClosure+debug.m \
			BoxedFloat.m \
			LanguageKitExceptions.m \
			MsgSendSmallInt.m \
			NSString+conversions.m \
			NSValue+structs.m \
			OverflowHandler.m \
			Symbol.m	 \

.include <bsd.kmod.mk>


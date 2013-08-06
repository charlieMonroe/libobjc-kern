CC=~/build/Debug+Asserts/bin/clang


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -wobjc-root-class
CFLAGS	+= -O0
CFLAGS	+= -msoft-float
CFLAGS += -lsoft-fp

LDFLAGS += -T
LDFLAGS += objc.lds

KMOD	= libobjc

SRCS	= kernel_module.c \
		arc.c \
		associative.c \
		category.c \
		class.c \
		class_extra.c \
		class_registry.c \
		dtable.c \
		encoding.c \
		malloc_types.c \
		message.c \
		method.c \
		loader.c \
		objc_msgSend.S \
		property.c \
		protocol.c \
		runtime.c \
		selector.c \
		sarray2.c \
		KKObjects.m \
		test/ao-test.m \
		test/handmade-class-test.m \
		test/ivar-test.m \
		test/test.c \
		test/weak-ref-test.m
#		test/compiler-test.m 
		
.include <bsd.kmod.mk>


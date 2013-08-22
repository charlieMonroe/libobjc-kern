CC=~/build/Debug+Asserts/bin/clang


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0

LDFLAGS += --print-gc-sections

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
		exception.c \
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
		test/weak-ref-test.m \
		test/compiler-test.m \
		test/exception-test.m \
		test/forwarding-test.m \
		test/many-selectors-test.m

.include <bsd.kmod.mk>


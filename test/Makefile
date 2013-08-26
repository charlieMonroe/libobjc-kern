CC=~/build/Debug+Asserts/bin/clang


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0

LDFLAGS += --print-gc-sections

KMOD	= libobjc_test

SRCS	= ao-test.m \
		handmade-class-test.m \
		ivar-test.m \
		objc-test.c \
		weak-ref-test.m \
		compiler-test.m \
		exception-test.m \
		forwarding-test.m \
		message-send-test.m \
		property-test.m \
		property-introspection-test.m \
		property-introspection2-test.m \
		protocol-creation-test.m \
		runtime-test.m \
		many-selectors-test.m \
		category-test.m \
    protocol-as-object-test.m \
		load-test.m

.include <bsd.kmod.mk>


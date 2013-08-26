CC=~/build/Debug+Asserts/bin/clang


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0

LDFLAGS += --print-gc-sections

KMOD	= libobjc_test

SRCS	= test/ao-test.m \
		test/handmade-class-test.m \
		test/ivar-test.m \
		test/objc-test.c \
		test/weak-ref-test.m \
		test/compiler-test.m \
		test/exception-test.m \
		test/forwarding-test.m \
		test/message-send-test.m \
		test/property-test.m \
		test/property-introspection-test.m \
		test/property-introspection2-test.m \
		test/protocol-creation-test.m \
		test/runtime-test.m \
		test/many-selectors-test.m

.include <bsd.kmod.mk>


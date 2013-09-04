CC=~/build/Debug+Asserts/bin/clang

CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0
CFLAGS	+= -fblocks

LDFLAGS += --print-gc-sections

KMOD	= foundation_test

SRCS	= foundation-test.c \
		NSArrayTest.m \
		NSDictionaryTest.m \
		NSEnumeratorTest.m \
		NSIndexSetTest.m \
		NSNumberTest.m \
		NSStringTest.m \
		NSValueTest.m

.include <bsd.kmod.mk>


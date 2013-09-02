CC=~/build/Debug+Asserts/bin/clang


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0
CFLAGS	+= -fblocks

LDFLAGS += --print-gc-sections

KMOD	= my_class

SRCS	= module.c MyClass.m

.include <bsd.kmod.mk>


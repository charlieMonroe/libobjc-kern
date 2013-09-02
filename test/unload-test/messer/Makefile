CC=~/build/Debug+Asserts/bin/clang


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0
CFLAGS	+= -fblocks

LDFLAGS += --print-gc-sections

KMOD	= messer

SRCS	= module.c MyClassAdditions.m

.include <bsd.kmod.mk>


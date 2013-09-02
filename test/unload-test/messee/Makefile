CC=~/build/Debug+Asserts/bin/clang


CFLAGS  += -fobjc-runtime=kernel-runtime
CFLAGS	+= -O0
CFLAGS	+= -fblocks

LDFLAGS += --print-gc-sections

KMOD	= messee

SRCS	= module.c test.m

.include <bsd.kmod.mk>


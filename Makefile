CFLAGS  += -fobjc-runtime=kernel-runtime

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
		blocks.c \
		string_allocator.c

.include <bsd.kmod.mk>


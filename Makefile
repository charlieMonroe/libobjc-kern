#CFLAGS  = "-v"

KMOD	= libobjc

SRCS	= kernel_module.c \
		associative.c \
		category.c \
		class.c \
		class_extra.c \
		class_registry.c \
		dtable.c \
		encoding.c \
		message.c \
		method.c \
		loader.c \
		objc_msgSend.S \
		property.c \
		protocol.c \
		runtime.c \
		selector.c \
		sarray2.c 
		
.include <bsd.kmod.mk>


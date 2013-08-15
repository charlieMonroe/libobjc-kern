CC=~/build/Debug+Asserts/bin/clang

CFLAGS='-fobjc-runtime=kernel-runtime -fno-strict-aliasing -D_KERNEL -DKLD_MODULE -nostdinc -I. -I@ -I@/contrib/altq -fno-common -fno-omit-frame-pointer'
CFLAGS="$CFLAGS -mno-omit-leaf-frame-pointer -mno-aes -mno-avc -mno-red-zone -mno-mmx -mno-sse -msoft-float -mcmodel=kernel -ffreestanding -fstack-protector"
CFLAGS="$CFLAGS -Qunused-arguments -fformat-extensions"

FILES='arc.c
	associative.c
	category.c
	class.c
	class_extra.c
	class_registry.c
	dtable.c
	encoding.c
	exception.c
	malloc_types.c
	message.c
	method.c
	loader.c
	objc_msgSend.S
	property.c
	protocol.c
	runtime.c
	selector.c
	sarray2.c
	KKObjects.m
	test/ao-test.m
	test/handmade-class-test.m
	test/ivar-test.m
	test/compiler-test.m
	test/test.c
	test/weak-ref-test.m
	test/exception-test.m'

$CC $CFLAGS -o libobjc.man.ko $FILES

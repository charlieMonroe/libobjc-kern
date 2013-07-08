
CFLAGS=-std=c99 -O3 -DUSES_C89=0 -DHAS_ALWAYS_INLINE_ATTRIBUTE=1 -DOBJC_USES_INLINE_FUNCTIONS=0
TESTMRFLAGS=-DOBJC_INLINE_CACHING=2
TESTCOCOAFLAGS=-DOBJC_CACHING=1 -Wno-objc-root-class -Wno-deprecated-objc-isa-usage
LINKER=`x=\`uname\`; \
		if [ $${x} = Darwin ]; then \
		  	echo ld; \
		fi`
LFLAGS=`x=\`uname\`; \
		if [ $${x} = Darwin ]; then \
		  	echo '-r -macosx_version_min 10.5'; \
		fi`


all : modular-runtime-tests cocoa-tests direct-test
	echo "Done all."

direct-test : test/direct-test.c
	cc $(CFLAGS) test/direct-test.c -o test/direct-test


cocoa-tests: allocation-cocoa-test ao-cocoa-test category-cocoa-test dispatch-cocoa-test forwarding-cocoa-test forwarding2-cocoa-test ivar-cocoa-test super-dispatch-cocoa-test
	echo "Done Cocoa run-time tests."

allocation-cocoa-test : test/allocation-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc test/allocation-cocoa-test.m -o test/allocation-cocoa-test
ao-cocoa-test : test/ao-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc test/ao-cocoa-test.m -o test/ao-cocoa-test
category-cocoa-test : test/category-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc test/category-cocoa-test.m -o test/category-cocoa-test
dispatch-cocoa-test : test/dispatch-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc test/dispatch-cocoa-test.m -o test/dispatch-cocoa-test
forwarding-cocoa-test : test/forwarding-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc -DOBJC_FORWARDING_TEST=1 -framework Foundation test/forwarding-cocoa-test.m -o test/forwarding-cocoa-test
forwarding2-cocoa-test : test/forwarding2-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc -DOBJC_FORWARDING_TEST=1 test/forwarding2-cocoa-test.m -o test/forwarding2-cocoa-test
ivar-cocoa-test : test/ivar-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc test/ivar-cocoa-test.m -o test/ivar-cocoa-test
super-dispatch-cocoa-test : test/super-dispatch-cocoa-test.m
	cc $(TESTCOCOAFLAGS) -lobjc test/super-dispatch-cocoa-test.m -o test/super-dispatch-cocoa-test



modular-runtime-tests : allocation-test ao-test category-test dispatch-test forwarding-test ivar-test super-dispatch-test
	echo "Done modular run-time tests."

allocation-test : static
	cc -L/usr/lib/ -L. -lobjc-runtime -flto $(TESTMRFLAGS) test/allocation-test.c -o test/allocation-test

ao-test : static
	cc -L/usr/lib/ -L. -lobjc-runtime -flto $(TESTMRFLAGS) test/ao-test.c -o test/ao-test

category-test : static
	cc -L/usr/lib/ -L. -lobjc-runtime -flto $(TESTMRFLAGS) test/category-test.c -o test/category-test

dispatch-test : static
	cc -L/usr/lib/ -L. -lobjc-runtime -flto $(TESTMRFLAGS) test/dispatch-test.c -o test/dispatch-test

forwarding-test : static
	cc -L/usr/lib/ -L. -lobjc-runtime -flto $(TESTMRFLAGS) test/forwarding-test.c -o test/forwarding-test

ivar-test : static
	cc -L/usr/lib/ -L. -lobjc-runtime -flto $(TESTMRFLAGS) test/ivar-test.c -o test/ivar-test

super-dispatch-test : static
	cc -L/usr/lib/ -L. -lobjc-runtime -flto $(TESTMRFLAGS) test/super-dispatch-test.c -o test/super-dispatch-test




static: class.o method.o runtime.o selector.o array.o holder.o ao.o categs.o posix.o MRObjects.o MRObjectMethods.o
	$(LINKER) class.o method.o runtime.o selector.o array.o holder.o ao.o categs.o posix.o MRObjects.o MRObjectMethods.o $(LFLAGS) -o libobjc-runtime.a



MRObjectMethods.o : classes/MRObjectMethods.c
	cc $(CFLAGS) -c classes/MRObjectMethods.c -o MRObjectMethods.o
MRObjects.o : classes/MRObjects.c
	cc $(CFLAGS) -c classes/MRObjects.c -o MRObjects.o
class.o : class.c
	cc $(CFLAGS) -c class.c
method.o : method.c
	cc $(CFLAGS) -c method.c
runtime.o : runtime.c private.h
	cc $(CFLAGS) -c runtime.c
selector.o : selector.c
	cc $(CFLAGS) -c selector.c
array.o : structs/array.c
	cc $(CFLAGS) -c structs/array.c -o array.o
holder.o : structs/holder.c
	cc $(CFLAGS) -c structs/holder.c -o holder.o
ao.o : extras/ao-ext.c
	cc $(CFLAGS) -c extras/ao-ext.c -o ao.o
categs.o : extras/categs.c
	cc $(CFLAGS) -c extras/categs.c -o categs.o
posix.o : extras/posix.c
	cc $(CFLAGS) -c extras/posix.c -o posix.o

clean: 
	rm *.o *.a objc_test_no_lto objc_test_with_lto objc_test_cocoa ; rm test/*-test


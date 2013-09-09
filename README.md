# Objective-C Runtime for (FreeBSD) kernel

### About

It is pretty much as the title says. An Objective-C runtime designed to run in the kernel, the FreeBSD kernel in particular, but porting it to any other kernel shouldn't be much work (see the chapter on porting). It is based on the GNUstep Objective-C runtime and even uses a few slightly modified source files from there (mostly those related to properties and dtable).

### Differences between runtimes

The kernel runtime introduces some new ideas as well as limitations. Firstly, `SEL` isn't a pointer anymore, it's a 16-bit integer that directly maps into the dtable. Hence you may get some warnings when comparing with `NULL`. The kernel runtime defines `null_selector`, which should be used for comparison.

Selectors are strictly typed, meaning that every selector used within the runtime must have known types. Also, no selector may be defined twice with two different types, i.e. having `-(void)hello;` and `-(id)hello;` will either cause a compile error if both are defined in the same compilation unit, or a runtime error if not.

Since the compiler doesn't know about types when compiling `@selector` expressions, a runtime function `sel_getNamed` is called, which looks whether a selector with such name is registered with the runtime and if so, it is returned. If not, this causes a panic.

Note that `sel_registerName` takes two parameters, name and types. This is simply because this function should really *register* a selector - if you need to get a selector, use `@selector`, or `sel_getNamed`. Registering a selector with the same name but different types causes a panic as well.

Exceptions are handled using `setjmp` - `longjmp`. I have tried to port libunwind to the kernel, but alas to no success. If you want to, you can have a whack at it here: https://github.com/charlieMonroe/libunwind-kern (there's a readme as well). But back to the point - the lack of stack unwinding generally means you *shouldn't* be doing any cleanup using the `__attribute__(cleanup(xyz))` since it won't get executed.

There is no support for Garbage Collection, however, ARC is supported.

If you are going to implement your own ARR methods, never forget to implement your own `-autorelease` method as you could get to an infinite loop!

I strongly discourage anyone from using protocols within the kernel as well since they are weird creatures of the runtime in the first place. Since they aren't classes (but instances of a class Protocol) and usually declared in a header file, the nature of how the kernel module gets compiled, you end up with one instance of the protocol in any file that includes the header declaring the protocol. This makes it quite easy in the user space, however, here in kernel as modules can be unloaded, it can lead to a lot of trouble and fixing this would require pretty much reworking how protocols work. It is definitely fine to use them as long as you keep them in one compilation unit, or use dynamically allocated ones.

### Installing the runtime

To install the runtime you need to have both `git` and `subversion` installed (the subversion is for getting llvm+clang, unless you are going to use the clang binary included). Enter the folder you want to install the runtime to and use `git clone https://github.com/charlieMonroe/libobjc-kern.git` - this will clone the repository into a `libobj-kern` folder.

You will find a few subdirectories in `libobj-kern`:
- __clang__ - this contains some source files from clang that needed to be modified as well as a pre-built clang binary.
- __Foundation__ - a few classes from the Foundation framework that you can used for compatibility reasons. It gets loaded as a separate module, so you don't need to use them.
- __kernobjc__ - a folder with public headers.
- __LanguageKitRuntime__ - the runtime from GNUstep's LanguageKit, slightly modified.
- __os__ - a folder with `kernel.h` and `user.h` files that are used on whether the runtime is being compiled for the kernel space or user space (I've been testing some features in user land since the debugging is much easier).
- __Smalltalk__ - a folder containing stuff necessary to run Smalltalk in the kernel (just like the LanguageKit's runtime, slightly modified version of the GNUstep's code).
- __test__ - a module that tests basic runtime capabilities.
- __test-foundation__ - a test module that tests that the Foundation classes work.

You need to be using a special build of clang. There is a pre-built binary included (`clang/clang.tgz` - you need to unarchive it), built on FreeBSD 10 - it may not work on your system. If it works, you can skip on the next two paragraphs.

If it doesn't work on your system, you need to compile it. Use this guide to check out clang: http://clang.llvm.org/get_started.html - but don't compile it yet, since you need to update the source with files in the `libobjc-kern/clang` directory first.

There's a `Install.sh` script that installs these files, however, you may need to modify some of them since the LLVM project changes quite quickly and they may be already dated. The installation script assumes that there is a `llvm` directory in the same directory as the `libobjkern` directory, however, you can specify your own path as an argument.

Unfortunately, you need to go further than just building the compiler, you need to replace the default `cc` with it, and not just by defining the `CC` env variable as that applies only to `.c` files according to the FreeBSD kernel module makefiles (which `.m` files aren't) - everything else falls back to `cc` - the easiest way is to do the following:

```
sudo mv /usr/bin/cc /usr/bin/_cc
sudo ln -s /path/to/clang/build/dir/build/Debug+Asserts/bin/clang /usr/bin/cc
```

Now you are pretty much all done.

### Hello World

Now we will build a sample Hello World kernel module in Objective-C. This assumes you already have compiled `clang` and installed it as the default `cc`.

##### hello_world.m
Create a new directory called `hello_world`. Create a new file `hello_world.m` and enter following:

```
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <kernobjc/runtime.h>

@interface HelloWorld : KKObject
-(void)sayHello;
@end

@implementation HelloWorld
-(void)sayHello{
	printf("Hello World!\n");
}
@end

static int event_handler(struct module *module, int event, void *arg) {
	int e = 0;
	switch (event) {
		case MOD_LOAD:
			_objc_load_kernel_module(module);
			
			HelloWorld *world = [[HelloWorld alloc] init];
			[world sayHello];
			[world release];
			break;
		case MOD_UNLOAD:
			if (!_objc_unload_kernel_module(module)){
				e = EOPNOTSUPP;
			}
			break;
		default:
			e = EOPNOTSUPP;
			break;
	}
	return (e);
}

static moduledata_t hello_world_conf = {
	"hello_world", 	/* Module name. */
	event_handler,  /* Event handler. */
	NULL 		/* Extra data */
};

DECLARE_MODULE(hello_world, hello_world_conf, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
MODULE_VERSION(hello_world, 0);

/* Depend on libobjc */
MODULE_DEPEND(hello_world, libobjc, 0, 0, 999);
```

Generally each kernel module needs to have an event handler that is called on several occasions, most importantly on `MOD_LOAD` and `MOD_UNLOAD`. You need to call `_objc_load_kernel_module(module);` on `MOD_LOAD` manually - the runtime has no way of observing when a module is loaded, at least I am not aware of it. The same goes for `MOD_UNLOAD` - here, however, you __must__ check for the return value since the runtime might find some dependencies on classes declared in this module and it may "forbid" unloading the module. The module then __mustn't__ be unloaded (by returning `EOPNOTSUPP`) since it would most likely lead to page faults in the kernel and hence kernel panic sooner or later when the runtime tries to read from the class structures that get unmapped on module unload.

Then you need to declare the module data with the module name, declare the module and its version. You need to also declare the module's dependency on `libobjc` module. This is done only once per module, __not__ in every file.

```
NOTE: Technically, you may have more than one module per linker file. This is NOT supported by the runtime at this time. The runtime assumes one module per linker file! (linker file ~ the resulting .ko object)
```

##### Makefile
Now you need to create the `Makefile`:

```
CFLAGS	+= -fobjc-runtime=kernel-runtime
CFLAGS	+= -I/path/to/libobj-kern

KMOD	= hello_world

SRCS	= hello_world.m

.include <bsd.kmod.mk>
```

The first flag tells the compiler that you are targetting the kernel runtime, the second one adds the `libobjc-kern` directory to include paths, so that you can do `#import <kernobjc/runtime.h>` and actually even `#import <Foundation/Foundation.h>` (about that later).

`KMOD` is the name of your module and the rest is quite self-explanatory. Note that you __must__ include the `bsd.kmod.mk` file.

```
Note: If you want to use blocks, you need to add the following to the Makefile:

CFLAGS	+= -fblocks
```

##### Launching

First, in the `libobjc-kern` directory, check that `OBJC_DEBUG_LOG` is set to `0` in `os.h` - the logging is quite extensive. Now run the `make_and_load.sh` script. This script doesn't do anything else than `make && sudo make load` with a few file syncing operations in between. I found that playing with the kernel can result in quite frequent kernel panics, which unfortunately leads to lost files that weren't synced - to avoid this, I tend to use this script which in my experience fixed this issue, at least in most cases. It's a good idea to use something similar if you plan to experiment with the runtime and kernel since the files you are going to lose most likely are the ones edited last - which generally means the source code file you've modified.

Now go back to the `hello_world` directory and run `make && sudo make load` - your first ObjC kernel module. Run `sudo make unload` to unload it.

That's pretty much it and now you can do whatever you want!

### KKObject

As you might have noticed in the Hello World module, the `HelloWorld` class inherits from something called `KKObject` - this is a very light-weight version of `NSObject` - a simple root class. It includes the general memory management methods, as well as some other basic functionality - your classes are absolutely welcome to derive from this class. Note that unlike the traditional `NSObject`, the `KKObject` contains the retain count in a variable.

Aside from `KKObject`, the `KKObjects.h` header file declares two more classes.

`_KKConstString` is a class that implements ObjC constant strings. The class doesn't respond to any other messages other than `-cString` and `-length` - the Foundation (see next chapter) includes `NSString`, however, which extends the functionality of this class with all its methods.

`__KKUnloadedModuleException` is a class representing an exception caused by invoking an unloaded method. More about that in section on module unloading.

### Foundation

As mentioned before, the runtime comes with several subprojects, or submodules, Foundation being one of them. The Foundation is only a subset of the regular Foundation framework, containing only a few classes and those classes only implement methods found essential as well as required to port the LanguageKit runtime, etc.

Most notably, the Foundation implements `NSObject`, which however inherits from `KKObject` (!) - this means that the `NSObject` doesn't just have the `isa` field, but the `__retainCount` field as well. This shouldn't even be noticed, unless you plan to cast the object to something else, or your code relies on this fact.

The Foundation also implements `NSString` which automatically enriches the `_KKConstString` class with its methods, making those two compatible and allowing to use constant strings in a more meaningful way.

The other more "fully implemented" classes include `NSArray`, `NSDictionary`, `NSNumber` and `NSValue`. For `NSArray`, `NSDictionary` and `NSString`, their mutable counterparts are included as well.

If you intend to use any of the Foundation classes, you need to go to the `libobjc/Foundation` directory and run the `make_and_load.sh` script as well, since it is a separate kernel module. You also need to declare a dependency on the Foundation the same way you declared the dependency on `libobjc`:

```
/* Depend on Foundation */
MODULE_DEPEND(hello_world, objc_foundation, 0, 0, 999);
```

### Loading

As noted earlier, loading a kernel module is a simple `sudo make load` command. On the module load, you are required to to call the `_objc_load_kernel_module` function which finds the corresponding section in the loaded file and registers all the classes, categories and protocols with the runtime as well as registers the selectors and updates necessary internal structures.

The function returns `YES` if it finds some ObjC data in the module, `NO` if no data can be found.

### Unloading

With unloading the fun begins. Since loading the module really means just mapping some memory, unloading means unmapping it. This presents a grave danger in several scenarios:
- You have loaded a module that includes a category that adds some methods to a class implemented in another module (e.g. on `KKObject`), or even overrides some. As soon as your module gets unloaded, calling such a method would be a death trap since the memory containing the function implementing the method is no longer available, or in a worse case contains already something else.
- If someone creates a subclass of your class from another module, unloading your module would make that subclass "unstable", resulting in the same issue as the previous one.
- The runtime also doesn't copy class structures on load since they are externally visible symbols when linking and the class structures actually get linked together. This means that reading something from the class structure would again result in a kernel panic.

This is generally why you __must__ call `_objc_unload_kernel_module` on unload. As noted by the second point, however, someone may be still using some classes from your module, which makes it potentially dangerous to simply go ahead and unload the module. This is why you __must__ check the return value and if `NO` is returned, you __mustn't__ let the module unload.

So what exactly `_objc_unload_kernel_module` does do?

First, it checks whether there is a class allocated by another module that inherits from a class in your module. If yes, returns `NO`, otherwise goes on to unloading the module.

This means going through the class tree and removing all classes declared by this module. The same goes for protocols.

After this, the runtime goes through all the methods on the classes left and sees if any of the function pointers is from your module. If yes, the function pointer gets replaced by an internal function that throws a `__KKUnloadedModuleException` which contains both the object the message was sent to as well as the selector.

This behavior can be modified using a `objc_unloaded_module_method` hook that allows you to specify your own function to be called when an unloaded method is called.

Note that the runtime automatically restores default hooks when you unload a module that implements these hooks.

```
IMPORTANT: Make sure that no instances of the classes implemented in this module are left behind! The runtime can check all the classes, but cannot be responsible for any instances. If there are some globals, make sure you release them.
```

### Freeing memory from `*_copy*` functions

There are some functions in the runtime that return allocated memory that you are responsible for freeing afterwards, for example `class_copyIvarList`.

You must release it using the standard `free` function - the question is which kind. These kinds are described in `kernobjc/types.h` (at very the bottom) - the types are quite obvious.

### ObjC in Kernel (Considerations)

Obviously, being in kernel, it is important to know about when the runtime performs some allocations, or locking.

The runtime usually doesn't perform any allocations or locking when sending messages to objects. The only time that any allocations or locking is performed is when the very first message is sent to that particular object or class. At this point, class initialization is performed - the class is locked and the dispatch table is allocated and installed onto the class.

To force the class initialization on module load, simply implement a `+load` method on the class - the runtime will then immediately send the `+load` message to the class, thus forcing initialization.

Another point where any locking or allocations are performed is the `@synchronize` statement (or calling `objc_sync_enter`), which uses associated objects, which are implemented just like in GNUstep runtime by creating a fake class and installing a lock on there. This also means that associated objects can perform allocations and use some locks.

### Porting

Porting the runtime to another platform requires only to modify the `kernel.h` file in the `os` folder. This file contains all the OS-related stuff. Some is only defined using `#define` macros, other is declared using static inline functions. Which approach you use is very much up to you.

The runtime has currently only been tested on x86-64 architecture and if you want to support another one, you need to write your own `objc_msgSend` function, which is - for the sake of speed - written in assembly. See the x86-64 version for details on what it should be doing.
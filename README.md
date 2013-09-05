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

### Installing the runtime

To install the runtime you need to have both `git` and `subversion` installed (the subversion is for getting llvm+clang). Enter the folder you want to install the runtime to and use `git clone https://github.com/charlieMonroe/libobjc-kern.git` - this will clone the repository into a `libobj-kern` folder.

You will find a few subdirectories in `libobj-kern`:
- __clang__ - this contains some source files from clang that needed to be modified.
- __Foundation__ - a few classes from the Foundation framework that you can used for compatibility reasons. It gets loaded as a separate module, so you don't need to use them.
- __kernobjc__ - a folder with public headers.
- __LanguageKitRuntime__ - the runtime from GNUstep's LanguageKit, slightly modified.
- __os__ - a folder with `kernel.h` and `user.h` files that are used on whether the runtime is being compiled for the kernel space or user space (I've been testing some features in user land since the debugging is much easier).
- __test__ - a module that tests basic runtime capabilities.
- __test-foundation__ - a test module that tests that the Foundation classes work.

First, you will need to build your own compiler. Use this guide to check out clang: http://clang.llvm.org/get_started.html - but don't compile it yet, since you need to update the source with files in the `libobjc-kern/clang` directory first.

There's a `Install.sh` script that installs these files, however, you may need to modify some of them since the LLVM project changes quite quickly and they may be already dated. The installation script assumes that there is a `llvm` directory in the same directory as the `libobjkern` directory, however, you can specify your own path as an argument.

Unfortunately, you need to go further than just building the compiler, you need to replace the default `cc` with it, and not just by defining the `CC` env variable as that applies only to `.c` files according to the FreeBSD kernel module makefiles - everything else falls back to `cc` - the easiest way is to do the following:

`sudo mv /usr/bin/cc /usr/bin/_cc`

`sudo ln -s /path/to/clang/build/dir/build/Debug+Asserts/bin/clang /usr/bin/cc`

Now you are pretty much all done.

### Hello World

Now we will build a sample Hello World kernel module in Objective-C. This assumes you already have compiled `clang` and installed it as the default `cc`.

Create a new directory called `hello_world`. Create a new file `hello_world.m` and enter following:

```


```

First, in the `libobjc-kern` directory, run the `make_and_load.sh` script. This script doesn't do anything else than `make && sudo make load` with a few file syncing operations inbetween. I found that playing with the kernel can result in quite frequent kernel panics, which unfortunately lead to lost files that weren't synced - to avoid this, I tend to use this script which in my experience fixed this issue, at least partially. It's a good idea to use something similar if you plan to experiment with the runtime and kernel since the file you are going to lose most likely is the edited one last - which means that it's more likely the source code file you've modified.

### Porting

Porting the runtime to another platform requires only to modify the `kernel.h` file in the `os` folder. This file contains all the OS-related stuff. Some is only defined using `#define` macros, other is declared using static inline functions. Which approach you use is very much up to you.

The runtime has currently only been tested on x86-64 architecture and if you want to support another one, you need to write your own `objc_msgSend` function, which is - for the sake of speed - written in assembly. See the x86-64 version for details on what it should be doing.
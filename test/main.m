//
//  main.m
//  run-time-test
//
//  Created by Charlie Monroe on 12/9/12.
//  Copyright (c) 2012 KO Entertainment. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <time.h>
#import <objc/objc.h>
#import <objc/runtime.h>
#import <objc/message.h>

@interface MyClass{
	id isa;
}

@end
@implementation MyClass

-(id)log{
	static int i = 0;
	i++;
	//printf("Instance method, hallelujah - self %p, _cmd %p", self, _cmd);
	return nil;
}

@end

@interface MySubclass : MyClass

@end
@implementation MySubclass



@end

static id _second_log(id self, SEL _cmd, ...){
	printf("HELL");
	return nil;
}

static void dispatch_test(void){
	id instance = class_createInstance(objc_getClass("MySubclass"), 0);
	printf("Created an object instance: %p\n", instance);
	
	clock_t c1, c2;
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		objc_msgSend(instance, sel_registerName("log"));
	}
	
	c2 = clock();
	
	printf("Apple run-time call took %f seconds.\n", ((double)c2 - (double)c1)/ (double)CLOCKS_PER_SEC);
}

static void allocation_test(void){
	clock_t c1, c2;
	Class cl = objc_getClass("MySubclass");
	c1 = clock();
	for (int i = 0; i < 10000000; ++i){
		id instance = class_createInstance(cl, 0);
		object_dispose(instance);
	}
	
	c2 = clock();
	
	printf("Apple run-time allocation took %f seconds.\n", ((double)c2 - (double)c1)/ (double)CLOCKS_PER_SEC);
}

int main(int argc, const char * argv[])
{
	dispatch_test();
	allocation_test();
	
	return 0;
}


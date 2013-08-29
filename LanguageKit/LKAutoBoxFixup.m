#import "../Foundation/Foundation.h"
#import "LanguageKit.h"
#import "../kernobjc/runtime.h"
#import "../types.h"

struct objc_slot* objc_get_slot(Class cls, SEL selector);

struct objc_slot* objc_method_type_fixup(Class cls, SEL
										 selector, struct objc_slot *result);
struct objc_slot* objc_method_type_fixup(Class cls, SEL
		selector, struct objc_slot *result)
{
	@autoreleasepool {
		id <LKCodeGenerator> jit = defaultJIT();
		const char *selName = sel_getName(selector);
		const char *selTypes = sel_getTypes(selector);
		[jit startModule: nil];
		[jit createCategoryWithName: @"Type_Fixup"
					   onClassNamed: [NSString stringWithUTF8String: class_getName(cls)]];
		if (class_isMetaClass(cls))
		{
			[jit beginClassMethod: [NSString stringWithUTF8String:selName]
				 withTypeEncoding: [NSString stringWithUTF8String:selTypes]
						arguments: [NSArray array]
						   locals: NULL];
		}
		else
		{
			[jit beginInstanceMethod: [NSString stringWithUTF8String:selName]
					withTypeEncoding: [NSString stringWithUTF8String:selTypes]
						   arguments: [NSArray array]
							  locals: NULL];
		}
		SEL correctSel = sel_registerName(selName, result->types);
		Method m = class_getInstanceMethod(cls, correctSel);
		int argc = method_getNumberOfArguments(m) - 2;
		void *argv[argc];
		for (int i=0 ; i<argc ; i++)
		{
			objc_log("*******-loadValueFromVariable:lexicalScopeAtDepth: not implemented!\n");
			// TODO UNKNONW METHOD
			// argv[i] = [jit loadValueFromVariable: i lexicalScopeAtDepth: 0];
		}
		void *ret = [jit sendMessage: [NSString stringWithUTF8String:selName]
							   types: [NSString stringWithUTF8String:result->types]
							toObject: [jit loadSelf]
							withArgs: argv
							   count: argc];
		if (selTypes[0] != 'v' && result->types[0] != 'v')
		{
			[jit setReturn: ret];
		}
		[jit endMethod];
		[jit endCategory];
		[jit endModule];
		return objc_get_slot(cls, selector);
	}
}


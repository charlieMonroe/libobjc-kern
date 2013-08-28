#import "LKVariableDecl.h"
#import "LKToken.h"
#import "LKObject.h"
#import "../Foundation/Foundation.h"

@implementation LKVariableDecl
- (LKVariableDecl*) initWithName: (LKToken*) declName
{
	SUPERINIT;
	ASSIGN(variableName, declName);
	return self;
}
+ (LKVariableDecl*) variableDeclWithName:(LKToken*) declName
{
	return [[self alloc] initWithName:declName];
}
- (NSString*) description
{
	return [NSString stringWithFormat:@"var %@", variableName];
}
- (void) setParent:(LKAST*)aParent
{
	[super setParent:aParent];
	LKSymbol *sym = [LKSymbol new];
	[sym setName: variableName];
	[sym setTypeEncoding: NSStringFromRuntimeString(@encode(LKObject))];
	[sym setScope: LKSymbolScopeLocal];
	[symbols addSymbol: sym];
}
- (NSString*)name
{
	return (NSString*)variableName;
}
- (BOOL) check { return YES; }
- (void*) compileWithGenerator: (id<LKCodeGenerator>)aGenerator
{
	return NULL;
}
@end

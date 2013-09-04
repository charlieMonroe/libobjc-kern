
#import "NSEnumerator.h"
#import "NSArray.h"

@implementation NSEnumerator

+(NSEnumerator*)enumeratorWithArray:(NSArray*)array{
	NSEnumerator *enumerator = [[[self alloc] init] autorelease];
	enumerator->_representedObject = [array copy];
	enumerator->_index = 0;
	return enumerator;
}


-(NSArray*)allObjects{
	return _representedObject;
}
-(NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState*)state
								 objects:(__unsafe_unretained id[])stackbuf
								   count:(NSUInteger)len{
	return [_representedObject countByEnumeratingWithState:state
												   objects:stackbuf
													 count:len];
}
-(void)dealloc{
	[_representedObject release];
	
	[super dealloc];
}
-(id)nextObject{
	if (_index >= [_representedObject count]){
		return nil;
	}
	id obj = [_representedObject objectAtIndex:_index];
	++_index;
	return obj;
}


@end

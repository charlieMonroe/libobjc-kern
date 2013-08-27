
#import "NSNull.h"

@implementation NSNull

+(NSNull*)null{
	static NSNull *_sharedNull;
	if (_sharedNull == nil){
		@synchronized(self){
			if (_sharedNull == nil){
				_sharedNull = [[self alloc] init];
			}
		}
	}
	return _sharedNull;
}

@end

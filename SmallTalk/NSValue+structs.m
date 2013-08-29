#import "../Foundation/Foundation.h"

@implementation NSValue (structures)
+ (id) rangeWithLocation:(int)loc length:(int)len
{
	return [NSValue valueWithRange:NSMakeRange(loc, len)];
}

+ (id) pointWithX:(NSUInteger)x Y:(NSUInteger)y
{
	return [NSValue valueWithPoint:NSMakePoint(x, y)];
}

+ (id) rectWithX:(NSUInteger)x Y:(NSUInteger)y width:(NSUInteger)width height:(NSUInteger)height
{
	return [NSValue valueWithRect:NSMakeRect(x, y, width, height)];
}

+ (id) sizeWithWidth:(NSUInteger)width height:(NSUInteger)height
{
	return [NSValue valueWithSize:NSMakeSize(width, height)];
}

- (NSUInteger) location
{
	return [self rangeValue].location;
}

- (NSUInteger) length
{
	return [self rangeValue].length;
}

- (NSUInteger) x
{
	return [self pointValue].x;
}

- (NSUInteger) y
{
	return [self pointValue].y;
}

- (NSValue *) origin
{
	return [NSValue valueWithPoint: [self rectValue].origin];
}

- (NSValue *) size
{
	return [NSValue valueWithSize: [self rectValue].size];
}

- (NSUInteger) width
{
	return [self sizeValue].width;
}

- (NSUInteger) height
{
	return [self sizeValue].height;
}
@end

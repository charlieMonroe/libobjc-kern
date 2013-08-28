//
//  NSValue.m
//  libkernobjc_xcode
//
//  Created by Charlie Monroe on 8/28/13.
//  Copyright (c) 2013 Krystof Vasa. All rights reserved.
//

#import "NSValue.h"
#import "NSException.h"

static NSString *const NSValueAbstractException = @"NSValueAbstractException";

#define NSValueCreateAndReturnPopulated(code)		\
			NSValue *value = [self value];			\
			code;									\
			return value;

#define NSValueAssertType(type)						\
			if (_type != type){						\
				@throw [NSException exceptionWithName:NSInternalInconsistencyException  \
												reason:@"" userInfo:nil];				\
			}

@implementation NSValue

+(NSValue*)value{
	return [[[self alloc] init] autorelease];
}
+(NSValue*)valueWithNonretainedObject:(id)anObject{
	NSValueCreateAndReturnPopulated({
		value->_data.pointer = anObject;
		value->_type = NSValueTypeNonretainedObject;
	});
}
+(NSValue*)valueWithPoint:(NSPoint)point{
	NSValueCreateAndReturnPopulated({
		value->_data.point = point;
		value->_type = NSValueTypePoint;
	});
}
+(NSValue*)valueWithPointer:(const void*)pointer{
	NSValueCreateAndReturnPopulated({
		value->_data.pointer = pointer;
		value->_type = NSValueTypePointer;
	});
}
+(NSValue*)valueWithRange:(NSRange)range{
	NSValueCreateAndReturnPopulated({
		value->_data.range = range;
		value->_type = NSValueTypeRange;
	});
}
+(NSValue*)valueWithRect:(NSRect)rect{
	NSValueCreateAndReturnPopulated({
		value->_data.rect = rect;
		value->_type = NSValueTypeRect;
	});
}
+(NSValue*)valueWithSize:(NSSize)size{
	NSValueCreateAndReturnPopulated({
		value->_data.size = size;
		value->_type = NSValueTypeSize;
	});
}

-(size_t)_sizeForCurrentType{
	switch (_type) {
		case NSValueTypeNonretainedObject:
			return sizeof(id);
		case NSValueTypePoint:
			return sizeof(NSPoint);
		case NSValueTypePointer:
			return sizeof(const void*);
		case NSValueTypeRange:
			return sizeof(NSRange);
		case NSValueTypeRect:
			return sizeof(NSRect);
		case NSValueTypeSize:
			return sizeof(NSSize);
		default:
			break;
	}
	
	@throw [NSException exceptionWithName:NSValueAbstractException reason:@"" userInfo:nil];
}

-(void)getValue:(void*)value{
	memcpy(value, &_data, [self _sizeForCurrentType]);
}

-(id)nonretainedObjectValue{
	NSValueAssertType(NSValueTypeNonretainedObject);
	return (id)_data.pointer;
}
-(void*)pointerValue{
	NSValueAssertType(NSValueTypePointer);
	return (void*)_data.pointer;
}
-(NSRange)rangeValue{
	NSValueAssertType(NSValueTypeRange);
	return _data.range;
}
-(NSRect)rectValue{
	NSValueAssertType(NSValueTypeRect);
	return _data.rect;
}
-(NSSize)sizeValue{
	NSValueAssertType(NSValueTypeSize);
	return _data.size;
}
-(NSPoint)pointValue{
	NSValueAssertType(NSValueTypePoint);
	return _data.point;
}

@end





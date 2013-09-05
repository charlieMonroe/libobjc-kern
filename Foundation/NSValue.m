

#import "NSValue.h"
#import "NSException.h"
#import "../utils.h"

static NSString *const NSValueAbstractException = @"NSValueAbstractException";

#define NSValueCreateAndReturnPopulated(typeEnc, code)		\
			NSValue *value = [self value];					\
			value->_objCType = typeEnc;						\
			code;											\
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
+(NSValue *)valueWithBytes:(void *)bytes objCType:(const char *)type{
	#define __setIntegralValueOfType(type) \
			value->_data.signedIntegralValue = ((type*)bytes)[0]; \
			value->_type = NSValueTypeIntegral; \
			break;
	#define __setUnsignedIntegralValueOfType(type) \
			value->_data.unsignedIntegralValue = ((unsigned type*)bytes)[0]; \
			value->_type = NSValueTypeIntegral; \
			break;
	NSValue *value = [self value];
	switch(*type){
		case 'B':
		case 'c':
			__setIntegralValueOfType(BOOL);
		case 'C':
			__setUnsignedIntegralValueOfType(char);
		case 's':
			__setIntegralValueOfType(short);
		case 'S':
			__setUnsignedIntegralValueOfType(short);
		case 'i':
			__setIntegralValueOfType(int);
		case 'I':
			__setUnsignedIntegralValueOfType(int);
		case 'l':
			__setIntegralValueOfType(long);
		case 'L':
			__setUnsignedIntegralValueOfType(long);
		case 'q':
			__setIntegralValueOfType(long long);
		case 'Q':
			__setUnsignedIntegralValueOfType(long long);
		case ':':
			__setUnsignedIntegralValueOfType(short);
			break;
		case '{':
		{
			if (objc_string_has_prefix(type, "{_NSRect")){
				value->_data.rect = ((NSRect*)bytes)[0];
				value->_type = NSValueTypeRect;
			}else if (objc_string_has_prefix(type, "{_NSRange")){
				value->_data.range = ((NSRange*)bytes)[0];
				value->_type = NSValueTypeRange;
			}else if (objc_string_has_prefix(type, "{_NSPoint")){
				value->_data.point = ((NSPoint*)bytes)[0];
				value->_type = NSValueTypePoint;
			}else if (objc_string_has_prefix(type, "{_NSSize")){
				value->_data.size = ((NSSize*)bytes)[0];
				value->_type = NSValueTypeSize;
			}else{
				/* TODO Should be able to handle any struct. */
				[NSException raise: NSInternalInconsistencyException
							format: @"Unknown struct."];
			}
			break;
		}
		default:
			/* Assume pointer. */
			value->_data.pointer = ((void**)bytes)[0];
			value->_type = NSValueTypePointer;
			break;
	}
	
	value->_objCType = objc_strcpy(type);
	return value;
}
+(NSValue*)valueWithNonretainedObject:(id)anObject{
	NSValueCreateAndReturnPopulated(@encode(id), {
		value->_data.pointer = anObject;
		value->_type = NSValueTypeNonretainedObject;
	});
}
+(NSValue*)valueWithPoint:(NSPoint)point{
	NSValueCreateAndReturnPopulated(@encode(NSPoint), {
		value->_data.point = point;
		value->_type = NSValueTypePoint;
	});
}
+(NSValue*)valueWithPointer:(const void*)pointer{
	NSValueCreateAndReturnPopulated(@encode(const void*), {
		value->_data.pointer = pointer;
		value->_type = NSValueTypePointer;
	});
}
+(NSValue*)valueWithRange:(NSRange)range{
	NSValueCreateAndReturnPopulated(@encode(NSRange), {
		value->_data.range = range;
		value->_type = NSValueTypeRange;
	});
}
+(NSValue*)valueWithRect:(NSRect)rect{
	NSValueCreateAndReturnPopulated(@encode(NSRect), {
		value->_data.rect = rect;
		value->_type = NSValueTypeRect;
	});
}
+(NSValue*)valueWithSize:(NSSize)size{
	NSValueCreateAndReturnPopulated(@encode(NSSize), {
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

-(void)dealloc{
	// objc_dealloc((void*)_objCType, M_UTILITIES_TYPE);
	
	[super dealloc];
}

-(void)getValue:(void*)value{
	memcpy(value, &_data, [self _sizeForCurrentType]);
}

-(const char *)objCType{
	return _objCType;
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





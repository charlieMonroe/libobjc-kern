#import "../Foundation//NSValue.h"
#import "../utils.h"

id LKBoxValue(void *bytes, const char *typeEncoding)
{
	if (NULL == bytes)
	{
		return nil;
	}
	return [NSValue valueWithBytes:bytes objCType:typeEncoding];
}
void LKUnboxValue(id boxed, void *buffer, const char *typeEncoding)
{
	if (nil == boxed)
	{
		NSUInteger size, align;
		NSGetSizeAndAlignment(typeEncoding, &size, &align);
		memset(buffer, 0, size);
	}
	objc_assert(objc_strings_equal(typeEncoding, [boxed objCType]), "Wrong types\n");
	return [boxed getValue: buffer];
}


#ifndef OBJC_FOUNDATION_TYPES_H
#define OBJC_FOUNDATION_TYPES_H


/* Currently, we only support 64-bit computers. */
typedef unsigned long long	NSUInteger;
typedef long long			NSInteger;

/* Adding UTF8 support to the kernel probably doesn't make much sense, so in
 * the kernel Foundation, unichar is simply a char.
 */
typedef char unichar;

/* Zone is simply void. */
typedef void NSZone;

typedef enum {
	NSOrderedAscending = -1,
	NSOrderedSame,
	NSOrderedDescending
} NSComparisonResult;

typedef struct _NSRange {
	NSUInteger location;
	NSUInteger length;
} NSRange;

typedef struct _NSPoint {
	NSInteger x;
	NSInteger y;
} NSPoint;

typedef struct _NSSize {
	NSInteger width;
	NSInteger height;
} NSSize;

typedef struct _NSRect {
	NSPoint origin;
	NSSize	size;
} NSRect;


static inline NSRange NSMakeRange(NSUInteger location, NSUInteger length){
	NSRange r;
	r.location = location;
	r.length = length;
	return r;
}
static inline NSPoint NSMakePoint(NSUInteger x, NSUInteger y){
	NSPoint p;
	p.x = x;
	p.y = y;
	return p;
}
static inline NSSize NSMakeSize(NSUInteger width, NSUInteger height){
	NSSize s;
	s.width = width;
	s.height = height;
	return s;
}
static inline NSRect NSMakeRect(NSUInteger x, NSUInteger y, NSUInteger width,
								 NSUInteger height){
	NSRect r;
	r.origin.x = x;
	r.origin.y = y;
	r.size.width = width;
	r.size.height = height;
	return r;
}

static inline NSUInteger NSMaxRange(NSRange range) {
    return (range.location + range.length);
}

static inline BOOL NSLocationInRange(NSUInteger loc, NSRange range) {
    return (!(loc < range.location) && (loc - range.location) < range.length) ? YES : NO;
}

static inline BOOL NSEqualRanges(NSRange range1, NSRange range2) {
    return (range1.location == range2.location && range1.length == range2.length);
}

#define NSNotFound ((NSUInteger)-1)

#endif

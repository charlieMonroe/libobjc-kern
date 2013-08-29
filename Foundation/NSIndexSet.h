
#import "NSObject.h"
#import "NSTypes.h"

@interface NSIndexSet : NSObject {
	id			_data;
	NSUInteger	_dataCount; /* When 1, _data is NSValue, else NSArray */
	NSUInteger	_indexCount;
}

+(id)indexSet;
+(id)indexSetWithIndex:(NSUInteger)anIndex;
+(id)indexSetWithIndexesInRange:(NSRange)aRange;

-(BOOL)containsIndex:(NSUInteger)anIndex;
-(BOOL)containsIndexesInRange:(NSRange)aRange;
-(void)enumerateIndexesUsingBlock:(void (^)(NSUInteger idx, BOOL *stop))block;

-(NSUInteger)count;

-(NSUInteger)firstIndex;

-(id)initWithIndex:(NSUInteger)anIndex;
-(id)initWithIndexesInRange:(NSRange)aRange;

-(BOOL)isEqualToIndexSet:(NSIndexSet*)aSet;

-(NSUInteger)lastIndex;

@end


@interface NSMutableIndexSet : NSIndexSet

-(void)addIndex:(NSUInteger)anIndex;
-(void)addIndexes:(NSIndexSet*)aSet;
-(void)addIndexesInRange:(NSRange)aRange;

-(void)removeAllIndexes;
-(void)removeIndex:(NSUInteger)anIndex;
-(void)removeIndexes:(NSIndexSet*)aSet;
-(void)removeIndexesInRange:(NSRange)aRange;

@end
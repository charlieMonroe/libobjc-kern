//
//  NSMutableArray.h
//  libkernobjc_xcode
//
//  Created by Charlie Monroe on 8/27/13.
//  Copyright (c) 2013 Krystof Vasa. All rights reserved.
//

#import "NSArray.h"

@interface NSMutableArray : NSArray {
	NSUInteger	_capacity;
}

+(id)arrayWithCapacity:(NSUInteger)numItems;

-(void)addObject:(id)anObject;
-(void)addObjectsFromArray:(NSArray*)otherArray;

-(void)exchangeObjectAtIndex:(NSUInteger)i1 withObjectAtIndex:(NSUInteger)i2;

-(id)initWithCapacity:(NSUInteger)numItems;

-(void)insertObject:(id)anObject atIndex: (NSUInteger)index;

-(void)removeObjectAtIndex:(NSUInteger)index;

-(void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)anObject;

-(void)setArray:(NSArray*)otherArray;

-(void)removeAllObjects;
-(void)removeLastObject;
-(void)removeObject:(id)anObject;
-(void)removeObject:(id)anObject inRange:(NSRange)aRange;
-(void)removeObjectIdenticalTo:(id)anObject;
-(void)removeObjectIdenticalTo:(id)anObject inRange:(NSRange)aRange;
-(void)removeObjectsInArray:(NSArray*)otherArray;
-(void)removeObjectsInRange:(NSRange)aRange;

@end

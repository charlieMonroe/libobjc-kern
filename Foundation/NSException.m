//
//  NSException.m
//  libkernobjc_xcode
//
//  Created by Charlie Monroe on 8/27/13.
//  Copyright (c) 2013 Krystof Vasa. All rights reserved.
//

#import "NSException.h"

NSString *const NSInternalInconsistencyException = @"NSInternalInconsistencyException";

@implementation NSException

+(NSException *)exceptionWithName:(NSString *)name reason:(NSString *)reason userInfo:(id)userInfo{
	return [[[self alloc] initWithName:name reason:reason userInfo:userInfo] autorelease];
}

-(id)initWithName:(NSString *)name reason:(NSString *)reason userInfo:(id)userInfo{
	if ((self = [super init]) != nil){
		[self setName:name];
		[self setReason:reason];
		[self setUserInfo:userInfo];
	}
	return self;
}

-(void)dealloc{
	[self setName:nil];
	[self setReason:nil];
	[self setUserInfo:nil];
	
	[super dealloc];
}

-(void)raise{
	@throw self;
}

@end

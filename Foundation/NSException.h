//
//  NSException.h
//  libkernobjc_xcode
//
//  Created by Charlie Monroe on 8/27/13.
//  Copyright (c) 2013 Krystof Vasa. All rights reserved.
//

#import "NSObject.h"

@class NSString;

@interface NSException : NSObject

+ (NSException*) exceptionWithName: (NSString*)name
							reason: (NSString*)reason
						  userInfo: (id)userInfo;

- (id) initWithName: (NSString*)name
			 reason: (NSString*)reason
		   userInfo: (id)userInfo;


-(void)raise;

@property (readwrite, retain) NSString  *name;
@property (readwrite, retain) NSString  *reason;
@property (readwrite, retain) id        userInfo;

@end

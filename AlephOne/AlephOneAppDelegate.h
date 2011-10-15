//
//  AlephOneAppDelegate.h
//  AlephOne
//
//  Created by Robert Fielding on 10/14/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#import <UIKit/UIKit.h>

@class AlephOneViewController;

@interface AlephOneAppDelegate : NSObject <UIApplicationDelegate> {

}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet AlephOneViewController *viewController;

@end

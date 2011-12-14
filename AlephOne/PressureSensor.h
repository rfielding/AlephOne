//
//  PressureSensor.h
//  Pythagoras2
//
//  Created by Robert Fielding on 11/20/11.
//  Copyright 2011 None. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIGestureRecognizerSubclass.h>

#define CPBPressureNone         0.0f
#define CPBPressureLight        0.1f
#define CPBPressureMedium       0.3f
#define CPBPressureHard         0.8f
#define CPBPressureInfinite     2.0f

@interface PressureSensor : NSObject<UIAccelerometerDelegate> {
@public
float pressure;
float lastx;
float lasty;
float lastz;
float lastxNorm;
float lastyNorm;
float lastzNorm;
@private
}

@property (readonly, assign) float pressure;

-(void)setup;
-(void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration; 
@end
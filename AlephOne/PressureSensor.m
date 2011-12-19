//
//  PressureSensor.m
//  Pythagoras2
//
//  Created by Robert Fielding on 11/20/11.
//  Copyright 2011 None. All rights reserved.
//



#import <Foundation/Foundation.h>
#import <UIKit/UIGestureRecognizerSubclass.h>

float PressureSensor_pressure=1;
float PressureSensor_xNorm=0;
float PressureSensor_yNorm=0;
float PressureSensor_zNorm=1;


@interface PressureSensor : NSObject<UIAccelerometerDelegate> {
@private
    float lastx;
    float lasty;
    float lastz;
}


-(void)setup;
-(void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration; 
@end

#define kUpdateFrequency            20.0f
#define KNumberOfPressureSamples    3


@implementation PressureSensor

- (id)init {
    self = [super init];
    if (self != nil) {
        [self setup];
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)setup {
    NSLog(@"PressureSensor setup");
    PressureSensor_pressure = 0.0;
    
    [[UIAccelerometer sharedAccelerometer] setUpdateInterval:1.0f / kUpdateFrequency];
    [[UIAccelerometer sharedAccelerometer] setDelegate:self];
}

#pragma -
#pragma UIAccelerometerDelegate methods
-(void)accelerometer:(UIAccelerometer *)accelerometer didAccelerate:(UIAcceleration *)acceleration {
    float dx = acceleration.x - lastx;
    float dy = acceleration.y - lasty;
    float dz = acceleration.z - lastz;
    float v = sqrtf(dx*dx + dy*dy + dz*dz);
    float n = 
        sqrtf(
              acceleration.x*acceleration.x + 
              acceleration.y*acceleration.y + 
              acceleration.z*acceleration.z);
    lastx = acceleration.x;
    lasty = acceleration.y;
    lastz = acceleration.z;
    PressureSensor_xNorm = acceleration.x /n;
    PressureSensor_yNorm = acceleration.y /n;
    PressureSensor_zNorm = acceleration.z /n;
    PressureSensor_pressure = v;
}



@end

void PressureSensor_setup()
{
    [[PressureSensor alloc] init];    
}

//
//  PressureSensor.m
//  Pythagoras2
//
//  Created by Robert Fielding on 11/20/11.
//  Copyright 2011 None. All rights reserved.
//


#import "PressureSensor.h"

#define kUpdateFrequency            30.0f
#define KNumberOfPressureSamples    3


@implementation PressureSensor
@synthesize pressure;


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
    pressure = CPBPressureNone;
    
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
    lastxNorm = acceleration.x / n;
    lastyNorm = acceleration.y / n;
    lastzNorm = acceleration.z / n;
    pressure = v;
}


- (void)reset {
    pressure = CPBPressureNone;
}

@end

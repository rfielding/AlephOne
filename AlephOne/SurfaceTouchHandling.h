//
//  SurfaceTouchHandling.h
//  AlephOne
//
//  Created by Robert Fielding on 12/22/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct PitchHandler_context;
struct Fretless_context;

void SurfaceTouchHandling_touchesFlush();
void SurfaceTouchHandling_touchesInit(
                                      struct PitchHandler_context* phctxArg, 
                                      struct Fretless_context* fctxArg,
                                      int (*failArg)(const char*,...),
                                      int (*loggerArg)(const char*,...)
                                      );
void SurfaceTouchHandling_touchesUp(int finger,void* touch);
void SurfaceTouchHandling_touchesDown(int finger,void* touch,int isMoving,float x,float y, float velocity, float area);
void SurfaceTouchHandling_tick();
float SurfaceTouchHandling_getChorusLevel();
void SurfaceTouchHandling_setChorusLevel(float chorus);
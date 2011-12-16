//
//  GenericTouchHandling.h
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct PitchHandlerContext;
struct Fretless_context;

void GenericTouchHandling_touchesInit(
    struct PitchHandlerContext* pctxArg,
    struct Fretless_context* fctxArg,
    int (*failArg)(const char*,...),
    int (*loggerArg)(const char*,...)
);
void GenericTouchHandling_touchesUp(void* touch);
void GenericTouchHandling_touchesDown(void* touch,int isMoving,float x,float y,float velocity,float area);
void GenericTouchHandling_touchesFlush();
void GenericTouchHandling_tick();    

//Chorus is not a post-processing effect, but a voice duplication, so it's handled here
float GenericTouchHandling_getChorusLevel();
void GenericTouchHandling_setChorusLevel(float chorus);
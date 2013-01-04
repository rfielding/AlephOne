//
//  GenericTouchHandling.h
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Rob Fielding Software.
//
/**
   This code is the intermediary between the UIView and the PitchHandler and Fretless modules.
   It mostly forwards touch handling into the PitchHandler API, and does transformations
   like chorus as it does so.
 
   PitchHandler turns raw gestures into intentions that need information on scales and fretting patterns.
   When it gets this information, it gives it to Fretless to render the actual raw pitches.
 */

struct PitchHandler_context;
struct Fretless_context;

void GenericTouchHandling_touchesInit(
    struct PitchHandler_context* pctxArg,
    struct Fretless_context* fctxArg,
    int (*failArg)(const char*,...),
    int (*loggerArg)(const char*,...)
);
void GenericTouchHandling_touchesUp(void* touch);
void GenericTouchHandling_touchesDown(void* touch,int isMoving,float x,float y,float area);
void GenericTouchHandling_touchesFlush();
void GenericTouchHandling_tick();    

//Chorus is not a post-processing effect, but a voice duplication, so it's handled here
float GenericTouchHandling_getChorusLevel();
void GenericTouchHandling_setChorusLevel(float chorus);
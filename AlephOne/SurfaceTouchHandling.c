//
//  SurfaceTouchHandling.c
//  AlephOne
//
//  Created by Robert Fielding on 12/22/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


#include "SurfaceTouchHandling.h"

#include "Fretless.h"
#include "TouchMapping.h"
#include "PitchHandler.h"

#define FINGERMAX 16
#define NULL ((void*)0)


static float chorusLevel = 0.25;

static struct Fretless_context* fretlessp = NULL;
static struct PitchHandler_context* phctx = NULL;
static int (*fail)(const char*,...);
static int (*logger)(const char*,...);

void SurfaceTouchHandling_touchesFlush(void* ctx)
{
    Fretless_flush(fretlessp);    
}

void SurfaceTouchHandling_touchesInit(
                                      struct PitchHandler_context* phctxArg, 
                                      struct Fretless_context* fctxArg,
                                      int (*failArg)(const char*,...),
                                      int (*loggerArg)(const char*,...)
                                      )
{
    phctx = phctxArg;
    fretlessp = fctxArg;
    fail = failArg;
    logger = loggerArg;
    
    Fretless_boot(fretlessp);     
    
    SurfaceTouchHandling_setChorusLevel(0.25);    
}

void SurfaceTouchHandling_touchesUp(void* ctx,int finger,void* touch)
{
    if(finger < 0)
    {
        fail("touch did not map to a finger1");   
    }
    int finger2 = TouchMapping_mapFinger2(touch);
    if(finger < 0)
    {
        fail("touch did not map to a finger2");   
    }
    PitchHandler_unpickPitch(phctx,finger);
    Fretless_up(fretlessp, finger);
    Fretless_up(fretlessp, finger2);
    TouchMapping_unmapFinger2(touch);    
}


void SurfaceTouchHandling_touchesDown(void* ctx,int finger,void* touch,int isMoving,float x,float y, float velocity, float area)
{
    int finger1  = finger;
    if(finger1 < 0)
    {
        fail("touch did not map to a finger1\n");   
    }    
    int finger2  = TouchMapping_mapFinger2(touch);
    if(finger2 < 0)
    {
        fail("touch did not map to a finger2\n");   
    }    
    struct FingerInfo* fingerInfo = PitchHandler_pickPitch(phctx,finger1,isMoving,x,y);
    float note = fingerInfo->pitch;
    int polygroup = fingerInfo->string;
    float expr = fingerInfo->expr;
    int polyGroup1 = polygroup;
    int polyGroup2 = polygroup+8;
    float dx = (expr*expr*expr*expr)*chorusLevel;
    if(isMoving)
    {
        Fretless_move(fretlessp,finger1,note-dx,polyGroup1);
        Fretless_express(fretlessp, finger1, 0, expr);
        Fretless_move(fretlessp,finger2,note+dx,polyGroup2);
        Fretless_express(fretlessp, finger2, 0, expr);        
    }
    else
    {
        float velocity = 1.0*velocity*area;
        //logger("vel=%f\n",velocity);
        int legato = 0;
        Fretless_down(fretlessp,finger1, note-dx,polyGroup1,velocity,legato); 
        Fretless_express(fretlessp, finger1, 0, expr);
        Fretless_down(fretlessp,finger2,note+dx,polyGroup2,velocity,legato); 
        Fretless_express(fretlessp, finger2, 0, expr);        
    }
}

void SurfaceTouchHandling_tick(void* ctx)
{
    for(int finger=0; finger<FINGERMAX; finger++)
    {
        //Only the real finger will show up as active
        struct FingerInfo* fingerInfo = PitchHandler_fingerState(phctx,finger);
        if(fingerInfo->isActive)
        {
            int finger2 = TouchMapping_finger2FromFinger1(finger);
            float expr = fingerInfo->expr;
            float dx = (expr*expr*expr*expr)*chorusLevel;
            PitchHandler_pickPitch(phctx,finger,1,fingerInfo->fingerX,fingerInfo->fingerY);
            Fretless_move(fretlessp,finger,fingerInfo->pitch-dx,fingerInfo->string);            
            Fretless_move(fretlessp,finger2,fingerInfo->pitch+dx,fingerInfo->string);            
        }            
    }
    Fretless_flush(fretlessp);
    PitchHandler_tick(phctx);
}

float SurfaceTouchHandling_getChorusLevel()
{
    return chorusLevel;
}

void SurfaceTouchHandling_setChorusLevel(float chorus)
{
    chorusLevel = chorus;
}


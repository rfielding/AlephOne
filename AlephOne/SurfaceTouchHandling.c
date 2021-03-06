//
//  SurfaceTouchHandling.c
//  AlephOne
//
//  Created by Robert Fielding on 12/22/11.
//  Copyright 2011 Rob Fielding Software.
//


#include "SurfaceTouchHandling.h"

#include "Fretless.h"
#include "TouchMapping.h"
#include "PitchHandler.h"
#include "FretlessCommon.h"
#include "Parameters.h"

static float chorusLevelDesired = 0.1;
static float chorusLevel = 0;
static float baseVolume = 0.75;
static int legato = 2;
static int poly = 1;

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
    
    //SurfaceTouchHandling_setChorusLevel(0.25);    
}

void SurfaceTouchHandling_touchesUp(void* ctx,int finger,void* touch)
{
    if(finger < 0)
    {
        fail("touch did not map to a finger1");   
    }
    PitchHandler_unpickPitch(phctx,finger);
    Fretless_up(fretlessp, finger, legato);
}


void SurfaceTouchHandling_touchesDown(void* ctx,int finger,void* touch,int isMoving,float x,float y, float area)
{
    int finger1  = finger;
    if(finger1 < 0)
    {
        fail("touch did not map to a finger1\n");   
    } 
    
    struct FingerInfo* fingerInfo = PitchHandler_pickPitch(phctx,finger1,isMoving,x,y);
    float note = fingerInfo->pitch;
    int polygroup = fingerInfo->string;
    float expr = fingerInfo->expr;
    
    //Polyphony type is manipulating the polyphony mode
    int polyGroup1;
    switch(poly)
    {
        case 0: polyGroup1 = 0; break;
        case 1: polyGroup1 = polygroup; break;
        case 2: polyGroup1 = finger; break;
        default: polyGroup1 = 0;
    }
    
    //float e = expr;
    float v = (area * getSensitivity() + 1 * (1-getSensitivity())) * baseVolume;
    fingerInfo->velocity = v;
    if(isMoving)
    {
        Fretless_move(fretlessp,finger1,note,v,polyGroup1);
        Fretless_express(fretlessp, finger1, 11, expr);
    }
    else
    {
        Fretless_beginDown(fretlessp,finger1); 
        Fretless_express(fretlessp, finger1, 11, expr);
        Fretless_endDown(fretlessp,finger1, note,polyGroup1,v,legato?2:0); 
    }
}

void SurfaceTouchHandling_tick(void* ctx)
{
    int activeFingers = 0;
    for(int finger=0; finger<FINGERMAX; finger++)
    {
        //Only the real finger will show up as active
        struct FingerInfo* fingerInfo = PitchHandler_fingerState(phctx,finger);
        if(fingerInfo->isActive)
        {
            activeFingers++;
            //float expr = fingerInfo->expr;
            PitchHandler_pickPitch(phctx,finger,1,fingerInfo->fingerX,fingerInfo->fingerY);
            Fretless_move(fretlessp,finger,fingerInfo->pitch,fingerInfo->velocity,fingerInfo->string);    
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
    chorusLevelDesired = chorus;
}

float SurfaceTouchHandling_getBaseVolume()
{
    return baseVolume;
}

void SurfaceTouchHandling_setBaseVolume(float val)
{
    baseVolume = val;
}

void SurfaceTouchHandling_setLegato(int val)
{
    legato = val;
}

int SurfaceTouchHandling_getLegato()
{
    return legato;
}

void SurfaceTouchHandling_setPoly(int val)
{
    poly = val;
}

int SurfaceTouchHandling_getPoly()
{
    return poly;
}

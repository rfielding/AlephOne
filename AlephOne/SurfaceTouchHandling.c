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
#include "FretlessCommon.h"


static float chorusLevelDesired = 0.1;
static float chorusLevel = 0;
static float baseVolume = 1.0;
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
    int finger2;
    if(chorusLevel > 0)
    {
        finger2 = TouchMapping_mapFinger2(touch);        
        if(finger2 < 0)
        {
            fail("touch did not map to a finger2");   
        }
    }
    PitchHandler_unpickPitch(phctx,finger);
    Fretless_up(fretlessp, finger, legato);
    if(chorusLevel > 0)
    {
        Fretless_up(fretlessp, finger2, legato);
        TouchMapping_unmapFinger2(touch);            
    }
}


void SurfaceTouchHandling_touchesDown(void* ctx,int finger,void* touch,int isMoving,float x,float y, float velocity, float area)
{
    int finger1  = finger;
    if(finger1 < 0)
    {
        fail("touch did not map to a finger1\n");   
    } 
    int finger2;
    if(chorusLevel > 0)
    {
        finger2  = TouchMapping_mapFinger2(touch);
        if(finger2 < 0)
        {
            fail("touch did not map to a finger2\n");   
        }            
    }
    
    struct FingerInfo* fingerInfo = PitchHandler_pickPitch(phctx,finger1,isMoving,x,y);
    float note = fingerInfo->pitch;
    int polygroup = fingerInfo->string;
    float expr = fingerInfo->expr;
    
    //Polyphony type is manipulating the polyphony mode
    int polyGroup1;
    int polyGroup2;
    //Solo-mode with chorusing
    if(poly == 0)
    {
        polyGroup1 = 0;
        polyGroup2 = 8;
    }
    //Per-string polyphony
    if(poly == 1)
    {
        polyGroup1 = polygroup;
        polyGroup2 = (polygroup+8)%FINGERMAX;        
    }
    //Full polyphony
    if(poly == 2)
    {
        polyGroup1 = finger;
        polyGroup2 = finger2;
    }
    
    float e = expr;
    float dx = (e*e*e*e)*chorusLevel*0.0000000125;
    float v = baseVolume;
    fingerInfo->velocity = v;
    if(isMoving)
    {
        Fretless_move(fretlessp,finger1,note-dx,v,polyGroup1);
        Fretless_express(fretlessp, finger1, 11, expr);
        if(chorusLevel > 0)
        {
            Fretless_move(fretlessp,finger2,note+dx,v,polyGroup2);
            Fretless_express(fretlessp, finger2, 11, expr);                                
        }
    }
    else
    {
        //float v = area;
        //logger("v=%f velo=%f area=%f\n",v, velocity,area);
        Fretless_beginDown(fretlessp,finger1); 
        Fretless_express(fretlessp, finger1, 11, expr);
        Fretless_endDown(fretlessp,finger1, note-dx,polyGroup1,v,legato); 
        if(chorusLevel > 0)
        {
            Fretless_beginDown(fretlessp,finger2); 
            Fretless_express(fretlessp, finger2, 11, expr);                    
            Fretless_endDown(fretlessp,finger2,note+dx,polyGroup2,v,legato); 
        }
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
            int finger2;
            if(chorusLevel > 0)
            {
                finger2 = TouchMapping_finger2FromFinger1(finger);                
            }
            float expr = fingerInfo->expr;
            float dx = (expr*expr*expr*expr)*chorusLevel;
            PitchHandler_pickPitch(phctx,finger,1,fingerInfo->fingerX,fingerInfo->fingerY);
            Fretless_move(fretlessp,finger,fingerInfo->pitch-dx,fingerInfo->velocity,fingerInfo->string);    
            if(chorusLevel > 0)
            {
                Fretless_move(fretlessp,finger2,fingerInfo->pitch+dx,fingerInfo->velocity,fingerInfo->string);                            
            }
        }            
    }
    Fretless_flush(fretlessp);
    PitchHandler_tick(phctx);
    //It's only safe to change chorus state when all fingers are up
    if(activeFingers == 0 || (chorusLevel > 0 && chorusLevelDesired > 0))
    {
        chorusLevel = chorusLevelDesired;
        //We can't chorus with 1 channel, so just turn it off.
        if(activeFingers == 0 && Fretless_getMidiHintChannelSpan(fretlessp) <= 1)
        {
            chorusLevel = 0;
        }
    }
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

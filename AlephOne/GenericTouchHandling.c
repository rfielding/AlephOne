//
//  GenericTouchHandling.c
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "Fretless.h"
#include "GenericTouchHandling.h"
#include "TouchMapping.h"
#include "PitchHandler.h"
#include <stdio.h>
#include <stdlib.h>

static float chorusLevel = 0.25;

static struct Fretless_context* fretlessp = NULL;
static struct PitchHandlerContext* phctx = NULL;
static int (*fail)(const char*,...);
static int (*logger)(const char*,...);
static int lastNote = -1;

void GenericTouchHandling_touchesFlush()
{
    Fretless_flush(fretlessp);    
}

void GenericTouchHandling_touchesInit(
    struct PitchHandlerContext* phctxArg, 
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
    
    //Here mostly as an example, and to avoid having to tell people to set it up.  
    //12 should work just fine
    //Fretless_setMidiHintChannelBendSemis(fretlessp, 2);

    GenericTouchHandling_setChorusLevel(0.25);
    
}

void GenericTouchHandling_touchesUp(void* touch)
{
    int finger  = TouchMapping_mapFinger(touch);
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
    TouchMapping_unmapFinger(touch);
    TouchMapping_unmapFinger2(touch);    
}

void addIfNewNote(float note,float velocity)
{
    if((int)(note+0.5) != lastNote)
    {
        PitchHandler_addedANote(phctx,note,velocity);                    
    }
    lastNote = (int)(note+0.5);    
}

void GenericTouchHandling_touchesDown(void* touch,int isMoving,float x,float y, float velocity, float area)
{
    int finger1;
    int finger2;
    finger1  = TouchMapping_mapFinger(touch);
    if(finger1 < 0)
    {
        fail("touch did not map to a finger1");   
    }    
    finger2  = TouchMapping_mapFinger2(touch);
    if(finger2 < 0)
    {
        fail("touch did not map to a finger2");   
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

        addIfNewNote(note, velocity);
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
        
        addIfNewNote(note, velocity);
    }
}

void GenericTouchHandling_tick()
{
    for(int finger=0; finger<16; finger++)
    {
        int finger2 = TouchMapping_finger2FromFinger1(finger);
        //Only the real finger will show up as active
        struct FingerInfo* fingerInfo = PitchHandler_fingerState(phctx,finger);
        if(fingerInfo->isActive)
        {
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


float GenericTouchHandling_getChorusLevel()
{
    return chorusLevel;
}

void GenericTouchHandling_setChorusLevel(float chorus)
{
    chorusLevel = chorus;
}

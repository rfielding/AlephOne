//
//  GenericTouchHandling.c
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "Fretless.h"
#include "CoreMIDIRenderer.h"
#include "GenericTouchHandling.h"
#include "TouchMapping.h"
#include "PitchHandler.h"
#include <stdio.h>
#include <stdlib.h>

static float chorusLevel = 0.25;

static struct Fretless_context* fretlessp = NULL;
static struct PitchHandlerContext* phctx = NULL;

void GenericTouchHandling_touchesFlush()
{
    Fretless_flush(fretlessp);    
}

void GenericTouchHandling_touchesInit(struct PitchHandlerContext* phctxArg)
{
    phctx = phctxArg;
    
    //0.0 is C
    PitchHandler_clearFrets(phctx);

    float baseNote = 2.0; //D
    //First tetrachord
    PitchHandler_placeFret(phctx,baseNote + 0.0,4);
    PitchHandler_placeFret(phctx,baseNote + 1.0,2);
    PitchHandler_placeFret(phctx,baseNote + 1.5,1);
    PitchHandler_placeFret(phctx,baseNote + 2.0,2);
    PitchHandler_placeFret(phctx,baseNote + 3.0,3);
    PitchHandler_placeFret(phctx,baseNote + 4.0,2);
    //Second tetrachord
    PitchHandler_placeFret(phctx,baseNote + 0.0 + 5,4);
    PitchHandler_placeFret(phctx,baseNote + 1.0 + 5,2);
    PitchHandler_placeFret(phctx,baseNote + 1.5 + 5,1);
    //Tetrachord from fifth
    PitchHandler_placeFret(phctx,baseNote + 0.0 + 7,3);
    PitchHandler_placeFret(phctx,baseNote + 1.0 + 7,2);
    
    PitchHandler_placeFret(phctx,baseNote + 1.5 + 7,1);
    PitchHandler_placeFret(phctx,baseNote + 2.0 + 7,2);
    PitchHandler_placeFret(phctx,baseNote + 3.0 + 7,3);
    PitchHandler_placeFret(phctx,baseNote + 4.0 + 7,2);
    
    PitchHandler_setColCount(phctx,5);
    PitchHandler_setRowCount(phctx,3);
    PitchHandler_setNoteDiff(phctx,45); //A is bottom corner
    PitchHandler_setTuneSpeed(phctx,0.025);
    
    fretlessp = Fretless_init(CoreMIDIRenderer_midiPutch,CoreMIDIRenderer_midiFlush,malloc,CoreMIDIRenderer_midiFail,CoreMIDIRenderer_midiPassed,printf);
    
    CoreMIDIRenderer_midiInit(fretlessp);
    
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
        CoreMIDIRenderer_midiFail("touch did not map to a finger1");   
    }
    int finger2 = TouchMapping_mapFinger2(touch);
    if(finger < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger2");   
    }
    PitchHandler_unpickPitch(phctx,finger);
    Fretless_up(fretlessp, finger);
    Fretless_up(fretlessp, finger2);
    TouchMapping_unmapFinger(touch);
    TouchMapping_unmapFinger2(touch);    
}

void GenericTouchHandling_touchesDown(void* touch,int isMoving,float x,float y, float velocity, float area)
{
    int finger1;
    int finger2;
    finger1  = TouchMapping_mapFinger(touch);
    if(finger1 < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger1");   
    }    
    finger2  = TouchMapping_mapFinger2(touch);
    if(finger2 < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger2");   
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
        int legato = 0;
        Fretless_down(fretlessp,finger1, note-dx,polyGroup1,velocity,legato); 
        Fretless_express(fretlessp, finger1, 0, expr);
        Fretless_down(fretlessp,finger2,note+dx,polyGroup2,velocity,legato); 
        Fretless_express(fretlessp, finger2, 0, expr);        
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

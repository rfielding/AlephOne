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


static struct Fretless_context* fretlessp = NULL;

void GenericTouchHandling_touchesFlush()
{
    Fretless_flush(fretlessp);    
}

void GenericTouchHandling_touchesInit()
{
    fretlessp = Fretless_init(CoreMIDIRenderer_midiPutch,CoreMIDIRenderer_midiFlush,malloc,CoreMIDIRenderer_midiFail,CoreMIDIRenderer_midiPassed,printf);
    CoreMIDIRenderer_midiInit(fretlessp);
    //Fretless_setMidiHintChannelSpan(fretlessp, 1);
    Fretless_boot(fretlessp);     
}

void GenericTouchHandling_touchesUp(void* touch)
{
    int finger  = TouchMapping_mapFinger(fretlessp, touch);
    if(finger < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger1");   
    }
    int finger2 = TouchMapping_mapFinger2(fretlessp, touch);
    if(finger < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger2");   
    }
    PitchHandler_unpickPitch(finger);
    Fretless_up(fretlessp, finger);
    Fretless_up(fretlessp, finger2);
    TouchMapping_unmapFinger(fretlessp,touch);
    TouchMapping_unmapFinger2(fretlessp,touch);    
}

void GenericTouchHandling_touchesDown(void* touch,int isMoving,float x,float y)
{
    int finger1;
    int finger2;
    float noteHi;
    float noteLo;
    finger1  = TouchMapping_mapFinger(fretlessp, touch);
    if(finger1 < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger1");   
    }    
    finger2  = TouchMapping_mapFinger2(fretlessp, touch);
    if(finger2 < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger2");   
    }    
    struct FingerInfo* fingerInfo = PitchHandler_pickPitch(finger1,isMoving,x,y);
    float note = fingerInfo->pitch;
    int polygroup = fingerInfo->string;
    float expr = fingerInfo->expr;
    noteHi = note + (expr*expr)*0.2;
    noteLo = note - (expr*expr)*0.2;    
    int polyGroup1 = polygroup;
    int polyGroup2 = polygroup+8;
    if(isMoving)
    {
        Fretless_move(fretlessp,finger1,noteLo,polyGroup1);
        Fretless_express(fretlessp, finger1, 0, expr);
        Fretless_move(fretlessp,finger2,noteHi,polyGroup2);
        Fretless_express(fretlessp, finger2, 0, expr);        
    }
    else
    {
        float velocity = 1.0;
        int legato = 0;
        Fretless_down(fretlessp,finger1, noteLo,polyGroup1,velocity,legato); 
        Fretless_express(fretlessp, finger1, 0, expr);
        Fretless_down(fretlessp,finger2,noteHi,polyGroup2,velocity,legato); 
        Fretless_express(fretlessp, finger2, 0, expr);        
    }
}

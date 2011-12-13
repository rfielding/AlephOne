//
//  PitchHandler.c
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "PitchHandler.h"
#include <math.h>
#include <stdio.h>

#define FINGERMAX 16
#define NOBODY -1

struct PitchHandlerContext
{
    //static float tuneInterval = 5; //4.9804499913461244;  //12*log2f(4.0/3); //is Just intonation btw
    float tuneSpeed;
    float rowCount;
    float colCount;
    int noteDiff; //(48-1);

    float frets[1024];
    int fretsUsed;
    int fretIterator;
    float fretOffsetY;
    float fretOffsetX;
    float fretOffsetYInitial;
    float fretOffsetXInitial;

    int   lastFingerDown;
    float lastNoteDown;
    int   noteDiffOurs;
    float   noteDiffByFinger[FINGERMAX];
    float   pitchDiffByFinger[FINGERMAX];
    float   tuneInterval[FINGERMAX];

    int doOctaveRounding;
    struct FingerInfo fingers[FINGERMAX];
};

struct PitchHandlerContext* PitchHandler_init(void* (*allocFn)(unsigned long))
{
    struct PitchHandlerContext* ctx = 
        (struct PitchHandlerContext*)allocFn(sizeof(struct PitchHandlerContext));
    ctx->tuneSpeed = 0.1;
    ctx->rowCount = 3;
    ctx->colCount = 5;
    ctx->noteDiff = 48;
    ctx->fretsUsed = 0;
    ctx->fretIterator = 0;
    ctx->fretOffsetX = 0;
    ctx->fretOffsetY = 0;
    ctx->fretOffsetXInitial = 0.5;
    ctx->fretOffsetYInitial = 0.5;
    ctx->lastFingerDown = NOBODY;
    ctx->lastNoteDown = 0;
    ctx->noteDiffOurs = 0;
    ctx->doOctaveRounding = 1;
    for(int i=0; i<FINGERMAX; i++)
    {
        ctx->tuneInterval[i] = i*5;
        ctx->fingers[i].isActive = 0;
    }
    return ctx;
}

struct FingerInfo* PitchHandler_fingerState(struct PitchHandlerContext* ctx, int finger)
{
    return &ctx->fingers[finger];
}

int PitchHandler_getOctaveRounding(struct PitchHandlerContext* ctx)
{
    return ctx->doOctaveRounding;
}

void PitchHandler_setOctaveRounding(struct PitchHandlerContext* ctx, int octRound)
{
    ctx->doOctaveRounding = octRound;
}

float PitchHandler_getTuneInterval(struct PitchHandlerContext* ctx, int string)
{
    return ctx->tuneInterval[string];
}

void PitchHandler_setTuneInterval(struct PitchHandlerContext* ctx, int string,float tuning)
{
    ctx->tuneInterval[string] = tuning;
}

float PitchHandler_getTuneSpeed(struct PitchHandlerContext* ctx)
{
    return ctx->tuneSpeed;
}

void PitchHandler_setTuneSpeed(struct PitchHandlerContext* ctx, float tuneSpeedArg)
{
    ctx->tuneSpeed = tuneSpeedArg;
}

float PitchHandler_getRowCount(struct PitchHandlerContext* ctx)
{
    return ctx->rowCount;
}

void PitchHandler_setRowCount(struct PitchHandlerContext* ctx, float rowCountArg)
{
    ctx->rowCount = rowCountArg;
}
        
float PitchHandler_getColCount(struct PitchHandlerContext* ctx)
{
    return ctx->colCount;
}

void PitchHandler_setColCount(struct PitchHandlerContext* ctx, float colCountArg)
{
    ctx->colCount = colCountArg;
}

int PitchHandler_getNoteDiff(struct PitchHandlerContext* ctx)
{
    return ctx->noteDiff;
}

void PitchHandler_setNoteDiff(struct PitchHandlerContext* ctx, int noteDiffArg)
{
    ctx->noteDiff = noteDiffArg;
}

void PitchHandler_unpickPitch(struct PitchHandlerContext* ctx, int finger)
{
    ctx->fingers[finger].isActive = 0;
}

struct FingerInfo* PitchHandler_pickPitch(struct PitchHandlerContext* ctx, int finger,int isMoving,float x,float y)
{
    ctx->fingers[finger].string = (ctx->rowCount * y);
    ctx->fingers[finger].expr = (ctx->rowCount*y) - ctx->fingers[finger].string;
    float fret = ctx->colCount*x;
    ctx->fingers[finger].pitchRaw = (fret + ctx->tuneInterval[ctx->fingers[finger].string]); 
    
    ctx->fingers[finger].fingerX = x;
    ctx->fingers[finger].fingerY = y;
    ctx->fingers[finger].isActive = 1;    
    
    float thisPitch = ctx->fingers[finger].pitchRaw;
    

    
    if( isMoving )
    {
        ctx->noteDiffOurs = ctx->noteDiffByFinger[finger];
    }
    else
    {
        ctx->lastFingerDown = finger;
        ctx->noteDiffOurs = ctx->noteDiff;
        ctx->noteDiffByFinger[finger] = ctx->noteDiff;
    }

    
    thisPitch += ctx->noteDiffOurs;
    ctx->fingers[finger].beginPitch = thisPitch;
    ctx->fingers[finger].endPitch = PitchHandler_getTarget(ctx,thisPitch);
    
    float targetDrift = (ctx->fingers[finger].endPitch - thisPitch);
    if( isMoving )
    {
        float pitchDiff = fabs(ctx->fingers[finger].beginPitch - ctx->fingers[finger].endPitch);
        float tuneRate = ctx->tuneSpeed * pitchDiff;
        
        ctx->pitchDiffByFinger[finger] = (1 - tuneRate) * ctx->pitchDiffByFinger[finger] + tuneRate * targetDrift;                
    }
    else
    {
        ctx->pitchDiffByFinger[finger] = targetDrift;        
    }
    thisPitch += ctx->pitchDiffByFinger[finger];
    
    if(finger == ctx->lastFingerDown)
    {
        float diff = (thisPitch - ctx->lastNoteDown);
        if(ctx->doOctaveRounding)
        {
            if(diff > 6.5)
            {
                thisPitch -= 12;
                ctx->noteDiff -= 12;
                ctx->noteDiffOurs -= 12;
            }
            if(diff <= -6.5)
            {
                thisPitch += 12;
                ctx->noteDiff += 12;
                ctx->noteDiffOurs += 12;
            }            
        }
        while(thisPitch < -0.5)
        {
            thisPitch += 12;
            ctx->noteDiff += 12;
            ctx->noteDiffOurs += 12;
        }
        while(thisPitch >= 127.5)
        {
            thisPitch -= 12;
            ctx->noteDiff -= 12;
            ctx->noteDiffOurs -= 12;
        }
        ctx->lastNoteDown = thisPitch;
    }
    ctx->noteDiffByFinger[finger] = ctx->noteDiffOurs;        

    
    ctx->fingers[finger].pitchX = 
        ctx->fingers[finger].fingerX + (ctx->pitchDiffByFinger[finger] + 0.5)/ctx->colCount;
    ctx->fingers[finger].pitchY = ctx->fingers[finger].fingerY;
    ctx->fingers[finger].pitch = thisPitch;
    
    
    return &ctx->fingers[finger];
}


//Moveable fret generator
void PitchHandler_clearFrets(struct PitchHandlerContext* ctx)
{
    ctx->fretsUsed=0;
}

//This must span an octave from 0 <= pitch < 12.0, or everything breaks
void PitchHandler_placeFret(struct PitchHandlerContext* ctx, float pitch)
{
    ctx->frets[ctx->fretsUsed] = pitch;
    ctx->fretsUsed++;
}

void PitchHandler_getFretsBegin(struct PitchHandlerContext* ctx)
{
    ctx->fretIterator = -ctx->fretsUsed;
    ctx->fretOffsetY = ctx->fretOffsetYInitial;
    ctx->fretOffsetX = ctx->fretOffsetXInitial;
}

float PitchHandler_getPitchFromFret(struct PitchHandlerContext* ctx, int fret)
{
    int octave = (int)floorf(1.0 * fret / ctx->fretsUsed);
    return 12.0 * octave + ctx->frets[(fret+10*ctx->fretsUsed) % ctx->fretsUsed];
}

/**
 * TODO: this is inappropriate if we have a large number of frets per octave.
 * It should be a binary search in that case.
 */
float PitchHandler_getTarget(struct PitchHandlerContext* ctx, float pitch)
{
    int octaveEst = ctx->fretsUsed*floorf(pitch / 12.0);
    float pitchVal = pitch;
    float bestDistance=48;
    for(int fret=octaveEst - ctx->fretsUsed; fret <= octaveEst + ctx->fretsUsed; fret++)
    {
        float p = PitchHandler_getPitchFromFret(ctx,fret);
        float dist = fabs(pitch - ctx->fretOffsetXInitial - p);
        if(dist < bestDistance)
        {
            bestDistance = dist;
            pitchVal = p;
        }
    }
    return pitchVal;
}

int PitchHandler_getFret(struct PitchHandlerContext* ctx, float* pitch,float* x,float* y)
{
    float pitchVal = PitchHandler_getPitchFromFret(ctx,ctx->fretIterator) - ctx->noteDiff;
    
    if(pitchVal + ctx->fretOffsetX > ctx->colCount)
    {
        //Move back an octave in addition to the tuning interval
        //and move back an octave number of frets
        ctx->fretOffsetX -= ctx->tuneInterval[(int)ctx->fretOffsetY] + 12;
        //We went off the right edge of the screen
        //so move up a string
        ctx->fretOffsetY += 1;
        ctx->fretIterator -= ctx->fretsUsed;
        //notice that we ASSUME that the screen doesn't get wider than an octave here!
    }
    
    *pitch = pitchVal;
    *x = (pitchVal + ctx->fretOffsetX) / ctx->colCount;
    *y = (ctx->fretOffsetY) / ctx->rowCount;
    
    ctx->fretIterator++;
    return (ctx->fretOffsetY < ctx->rowCount+1);
}



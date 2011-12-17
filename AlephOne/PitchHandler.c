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
    int   fretImportance[1024];
    float fretUsage[1024];
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
    float   tuneIntervalCumulative[FINGERMAX];
    int doOctaveRounding;
    struct FingerInfo fingers[FINGERMAX];
    int (*fail)(const char*,...);
    int (*logger)(const char*,...);
};

struct PitchHandlerContext* PitchHandler_init(void* (*allocFn)(unsigned long),int (*fail)(const char*,...),int (*logger)(const char*,...))
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
    ctx->fail = fail;
    ctx->logger = logger;
    for(int i=0; i<FINGERMAX; i++)
    {
        PitchHandler_setTuneInterval(ctx, i, 5);
        ctx->fretUsage[i] = 0;
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
    
    //Set the cumulative tuning intervals
    ctx->tuneIntervalCumulative[0] = 0;
    for(int i=1; i<FINGERMAX; i++)
    {
        ctx->tuneIntervalCumulative[i] = ctx->tuneIntervalCumulative[i-1] + ctx->tuneInterval[i];        
    }
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
    ctx->fingers[finger].pitchRaw = (fret + ctx->tuneIntervalCumulative[ctx->fingers[finger].string]); 
    
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
    int fretPicked;
    ctx->fingers[finger].endPitch = PitchHandler_getTarget(ctx,thisPitch,&fretPicked);
 
    //Keep a histogram of fret usage
    fretPicked = (fretPicked+12*ctx->fretsUsed) % ctx->fretsUsed;
    //for(int f=0; f < ctx->fretsUsed; f++)
    //{
    //    ctx->fretUsage[f] *= (1.0*(ctx->fretsUsed-1))/ctx->fretsUsed;
    //}
    ctx->fretUsage[ fretPicked ] += 0.25;
    
    float targetDrift = (ctx->fingers[finger].endPitch - thisPitch);
    if( isMoving )
    {
        float tuneRate = ctx->tuneSpeed;// * fabs(targetDrift);
        
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


void PitchHandler_placeFret(struct PitchHandlerContext* ctx, float pitch, int importance)
{
    //Must be in range 0..12
    pitch = fmod(pitch,12);
    
    //Don't re-add existing values, but allow importance re-assign
    for(int f=0; f < ctx->fretsUsed; f++)
    {
        if(pitch == ctx->frets[f])
        {
            //Don't bother adding it, as it's already in here
            //But you can re-assign the importance
            ctx->fretImportance[f] = importance;
            return;
        }
    }
    
    ctx->frets[ctx->fretsUsed] = pitch;
    ctx->fretImportance[ctx->fretsUsed] = importance;
    
    int thisFret = ctx->fretsUsed;
    while(thisFret > 0 && ctx->frets[thisFret] < ctx->frets[thisFret-1])
    {
        //Swap and drop down one.  Bubble sort!
        float p = ctx->frets[thisFret];
        int i = ctx->fretImportance[thisFret];
        ctx->frets[thisFret] = ctx->frets[thisFret-1];
        ctx->fretImportance[thisFret] = ctx->fretImportance[thisFret-1];
        ctx->frets[thisFret-1] = p;
        ctx->fretImportance[thisFret-1] = i;
        thisFret--;
    }
    for(int f=0; f < ctx->fretsUsed; f++)
    {
        ctx->fretUsage[f] = 1.0/ctx->fretImportance[f];
    }
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
    return 12.0 * octave + ctx->frets[(fret+12*ctx->fretsUsed) % ctx->fretsUsed];
}

/**
 * TODO: this is inappropriate if we have a large number of frets per octave.
 * It should be a binary search in that case.
 */
float PitchHandler_getTarget(struct PitchHandlerContext* ctx, float pitch, int* fretP)
{
    int octaveEst = ctx->fretsUsed*floorf(pitch / 12.0);
    float pitchVal = pitch;
    float bestDistance=48;
    int fret;
    int bestFret;
    for(fret=octaveEst - ctx->fretsUsed; fret <= octaveEst + ctx->fretsUsed; fret++)
    {
        float p = PitchHandler_getPitchFromFret(ctx,fret);
        float dist = fabs(pitch - ctx->fretOffsetXInitial - p);
        if(dist < bestDistance)
        {
            bestDistance = dist;
            pitchVal = p;
            bestFret = fret;
        }
    }
    *fretP = bestFret;
    return pitchVal;
}

int PitchHandler_getFret(struct PitchHandlerContext* ctx, float* pitch,float* x,float* y,int* importance,float* usage)
{
    float pitchVal = PitchHandler_getPitchFromFret(ctx,ctx->fretIterator) - ctx->noteDiff;
    
    //If we have gone past the right edge of the screen
    if(pitchVal + ctx->fretOffsetX > ctx->colCount)
    {
        //and shift the string while moving to the next
        ctx->fretOffsetX -= ctx->tuneInterval[(int)ctx->fretOffsetY];
        ctx->fretOffsetY += 1;
        
        //go down an octave
        ctx->fretIterator -= ctx->fretsUsed;
        pitchVal -= 12;
        //ctx->fretOffsetX -= 12;
    }
    
    *pitch = pitchVal;
    *x = (pitchVal + ctx->fretOffsetX) / ctx->colCount;
    *y = (ctx->fretOffsetY) / ctx->rowCount;
    int ourFret = (ctx->fretIterator + 12*ctx->fretsUsed) % ctx->fretsUsed;
    *importance = ctx->fretImportance[ourFret];
    *usage = ctx->fretUsage[ourFret];
    ctx->fretIterator++;
    return *y < 1;
}

void PitchHandler_tick(struct PitchHandlerContext * ctx)
{
    //Keep a histogram of fret usage
    //int fretPicked = (fretPicked+12*ctx->fretsUsed) % ctx->fretsUsed;
    for(int f=0; f < ctx->fretsUsed; f++)
    {
        ctx->fretUsage[f] *= (1.0*(ctx->fretsUsed-1))/ctx->fretsUsed;
    }
    //ctx->fretUsage[ fretPicked ] += 0.25;    
}


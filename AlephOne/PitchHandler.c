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
#define FRETMAX 1024

struct Fret
{
    float cents[FRETMAX];
    int   importance[FRETMAX];
    float usage[FRETMAX];
    int used;
    int iterator;
    float offsetY;
    float offsetX;
    float offsetYInitial;
    float offsetXInitial;
};

struct PitchHandler_context
{
    //static float tuneInterval = 5; //4.9804499913461244;  //12*log2f(4.0/3); //is Just intonation btw
    float tuneSpeed;
    float rowCount;
    float colCount;
    int noteDiff; //(48-1);

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
    
    struct Fret fret;
};

struct PitchHandler_context* PitchHandler_init(void* (*allocFn)(unsigned long),int (*fail)(const char*,...),int (*logger)(const char*,...))
{
    struct PitchHandler_context* ctx = 
        (struct PitchHandler_context*)allocFn(sizeof(struct PitchHandler_context));
    ctx->tuneSpeed = 0.1;
    ctx->rowCount = 3;
    ctx->colCount = 5;
    ctx->noteDiff = 48;
    ctx->fret.used = 0;
    ctx->fret.iterator = 0;
    ctx->fret.offsetX = 0;
    ctx->fret.offsetY = 0;
    ctx->fret.offsetXInitial = 0.5;
    ctx->fret.offsetYInitial = 0.5;
    ctx->lastFingerDown = NOBODY;
    ctx->lastNoteDown = 0;
    ctx->noteDiffOurs = 0;
    ctx->doOctaveRounding = 1;
    ctx->fail = fail;
    ctx->logger = logger;
    for(int i=0; i<FINGERMAX; i++)
    {
        PitchHandler_setTuneInterval(ctx, i, 5);
        ctx->fret.usage[i] = 0;
    }
    return ctx;
}

struct FingerInfo* PitchHandler_fingerState(struct PitchHandler_context* ctx, int finger)
{
    return &ctx->fingers[finger];
}

int PitchHandler_getOctaveRounding(struct PitchHandler_context* ctx)
{
    return ctx->doOctaveRounding;
}

void PitchHandler_setOctaveRounding(struct PitchHandler_context* ctx, int octRound)
{
    ctx->doOctaveRounding = octRound;
}

float PitchHandler_getTuneInterval(struct PitchHandler_context* ctx, int string)
{
    return ctx->tuneInterval[string];
}

void PitchHandler_setTuneInterval(struct PitchHandler_context* ctx, int string,float tuning)
{
    ctx->tuneInterval[string] = tuning;
    
    //Set the cumulative tuning intervals
    ctx->tuneIntervalCumulative[0] = 0;
    for(int i=1; i<FINGERMAX; i++)
    {
        ctx->tuneIntervalCumulative[i] = ctx->tuneIntervalCumulative[i-1] + ctx->tuneInterval[i];        
    }
}

float PitchHandler_getTuneSpeed(struct PitchHandler_context* ctx)
{
    return ctx->tuneSpeed;
}

void PitchHandler_setTuneSpeed(struct PitchHandler_context* ctx, float tuneSpeedArg)
{
    ctx->tuneSpeed = tuneSpeedArg;
}

float PitchHandler_getRowCount(struct PitchHandler_context* ctx)
{
    return ctx->rowCount;
}

void PitchHandler_setRowCount(struct PitchHandler_context* ctx, float rowCountArg)
{
    ctx->rowCount = rowCountArg;
}
        
float PitchHandler_getColCount(struct PitchHandler_context* ctx)
{
    return ctx->colCount;
}

void PitchHandler_setColCount(struct PitchHandler_context* ctx, float colCountArg)
{
    ctx->colCount = colCountArg;
}

int PitchHandler_getNoteDiff(struct PitchHandler_context* ctx)
{
    return ctx->noteDiff;
}

void PitchHandler_setNoteDiff(struct PitchHandler_context* ctx, int noteDiffArg)
{
    ctx->noteDiff = noteDiffArg;
}

void PitchHandler_unpickPitch(struct PitchHandler_context* ctx, int finger)
{
    ctx->fingers[finger].isActive = 0;
}

struct FingerInfo* PitchHandler_pickPitch(struct PitchHandler_context* ctx, int finger,int isMoving,float x,float y)
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
    fretPicked = (fretPicked+12*ctx->fret.used) % ctx->fret.used;
    //for(int f=0; f < ctx->fretsUsed; f++)
    //{
    //    ctx->fretUsage[f] *= (1.0*(ctx->fretsUsed-1))/ctx->fretsUsed;
    //}
    ctx->fret.usage[ fretPicked ] += 0.25;
    
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

void PitchHandler_tick(struct PitchHandler_context * ctx)
{
    //Keep a histogram of fret usage
    //int fretPicked = (fretPicked+12*ctx->fretsUsed) % ctx->fretsUsed;
    for(int f=0; f < ctx->fret.used; f++)
    {
        ctx->fret.usage[f] *= (1.0*(ctx->fret.used-1))/ctx->fret.used;
    }
    //ctx->fretUsage[ fretPicked ] += 0.25;    
}

//Moveable fret generator
void PitchHandler_clearFrets(struct PitchHandler_context* ctx)
{
    ctx->fret.used=0;
}


void PitchHandler_placeFret(struct PitchHandler_context* ctx, float pitch, int importance)
{
    //Must be in range 0..12
    pitch = fmod(pitch,12);
    
    //Don't re-add existing values, but allow importance re-assign
    for(int f=0; f < ctx->fret.used; f++)
    {
        if(pitch == ctx->fret.cents[f])
        {
            //Don't bother adding it, as it's already in here
            //But you can re-assign the importance
            ctx->fret.importance[f] = importance;
            return;
        }
    }
    
    ctx->fret.cents[ctx->fret.used] = pitch;
    ctx->fret.importance[ctx->fret.used] = importance;
    
    int thisFret = ctx->fret.used;
    while(thisFret > 0 && ctx->fret.cents[thisFret] < ctx->fret.cents[thisFret-1])
    {
        //Swap and drop down one.  Bubble sort!
        float p = ctx->fret.cents[thisFret];
        int i = ctx->fret.importance[thisFret];
        ctx->fret.cents[thisFret] = ctx->fret.cents[thisFret-1];
        ctx->fret.importance[thisFret] = ctx->fret.importance[thisFret-1];
        ctx->fret.cents[thisFret-1] = p;
        ctx->fret.importance[thisFret-1] = i;
        thisFret--;
    }
    for(int f=0; f < ctx->fret.used; f++)
    {
        ctx->fret.usage[f] = 1.0/ctx->fret.importance[f];
    }
    ctx->fret.used++;
}

void PitchHandler_getFretsBegin(struct PitchHandler_context* ctx)
{
    ctx->fret.iterator = -ctx->fret.used;
    ctx->fret.offsetY = ctx->fret.offsetYInitial;
    ctx->fret.offsetX = ctx->fret.offsetXInitial;
}

float PitchHandler_getPitchFromFret(struct PitchHandler_context* ctx, int fret)
{
    int octave = (int)floorf(1.0 * fret / ctx->fret.used);
    return 12.0 * octave + ctx->fret.cents[(fret+12*ctx->fret.used) % ctx->fret.used];
}

/**
 * TODO: this is inappropriate if we have a large number of frets per octave.
 * It should be a binary search in that case.
 */
float PitchHandler_getTarget(struct PitchHandler_context* ctx, float pitch, int* fretP)
{
    int octaveEst = ctx->fret.used*floorf(pitch / 12.0);
    float pitchVal = pitch;
    float bestDistance=48;
    int fret;
    int bestFret;
    for(fret=octaveEst - ctx->fret.used; fret <= octaveEst + ctx->fret.used; fret++)
    {
        float p = PitchHandler_getPitchFromFret(ctx,fret);
        float dist = fabs(pitch - ctx->fret.offsetXInitial - p);
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

int PitchHandler_getFret(struct PitchHandler_context* ctx, float* pitch,float* x,float* y,int* importance,float* usage)
{
    float pitchVal = PitchHandler_getPitchFromFret(ctx,ctx->fret.iterator) - ctx->noteDiff;
    
    //If we have gone past the right edge of the screen
    if(pitchVal + ctx->fret.offsetX > ctx->colCount)
    {
        //and shift the string while moving to the next
        ctx->fret.offsetX -= ctx->tuneInterval[(int)ctx->fret.offsetY];
        ctx->fret.offsetY += 1;
        
        //go down an octave
        ctx->fret.iterator -= ctx->fret.used;
        pitchVal -= 12;
        //ctx->fretOffsetX -= 12;
    }
    
    *pitch = pitchVal;
    *x = (pitchVal + ctx->fret.offsetX) / ctx->colCount;
    *y = (ctx->fret.offsetY) / ctx->rowCount;
    int ourFret = (ctx->fret.iterator + 12*ctx->fret.used) % ctx->fret.used;
    *importance = ctx->fret.importance[ourFret];
    *usage = ctx->fret.usage[ourFret];
    ctx->fret.iterator++;
    return *y < 1;
}



/************
  Fretting subsection
 ************/
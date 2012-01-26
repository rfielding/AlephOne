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

#include "Fret.h"

#define FINGERMAX 16
#define NOBODY -1

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
    
    struct Fret_context* fret;
    
    int fretiterator;
    float fretoffsetY;
    float fretoffsetX;
    float fretoffsetYInitial;
    float fretoffsetXInitial;    
    int initSnap;
};

struct PitchHandler_context* PitchHandler_init(struct Fret_context* fctx,void* (*allocFn)(unsigned long),int (*fail)(const char*,...),int (*logger)(const char*,...))
{
    struct PitchHandler_context* ctx = 
        (struct PitchHandler_context*)allocFn(sizeof(struct PitchHandler_context));
    ctx->fret = fctx;
    ctx->tuneSpeed = 0.1;
    ctx->rowCount = 3;
    ctx->colCount = 5;
    ctx->noteDiff = 48;
    ctx->fretiterator = 0;
    ctx->fretoffsetX = 0;
    ctx->fretoffsetY = 0;
    ctx->fretoffsetXInitial = 0.5;
    ctx->fretoffsetYInitial = 0.5;
    ctx->lastFingerDown = NOBODY;
    ctx->lastNoteDown = 0;
    ctx->noteDiffOurs = 0;
    ctx->doOctaveRounding = 1;
    ctx->fail = fail;
    ctx->logger = logger;
    ctx->initSnap = 1;
    for(int i=0; i<FINGERMAX; i++)
    {
        ctx->fingers[i].isActive = 0;
        ctx->fingers[i].velocity = 0;
        PitchHandler_setTuneInterval(ctx, i, 5);
    }
    return ctx;
}

int PitchHandler_getSnap(struct PitchHandler_context* ctx)
{
    return ctx->initSnap;
}

void PitchHandler_setSnap(struct PitchHandler_context* ctx, int isSnap)
{
    ctx->initSnap = isSnap;
}

struct FingerInfo* PitchHandler_fingerState(struct PitchHandler_context* ctx, int finger)
{
    return &ctx->fingers[finger];
}

struct Fret_context* PitchHandler_frets(struct PitchHandler_context* ctx)
{
    return ctx->fret;
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

float PitchHandler_getStrDetune(struct PitchHandler_context* ctx, int str)
{
    return ctx->tuneInterval[str];
}




float PitchHandler_findStandardNote(struct PitchHandler_context* ctx, float x, float y)
{
    return (x * ctx->colCount) + ctx->tuneIntervalCumulative[(int)(y * ctx->rowCount)] + ctx->noteDiff;
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
    ctx->fingers[finger].endPitch = Fret_getTarget(ctx->fret,thisPitch,&fretPicked);
    if(ctx->fret->used > 0)
    {
        
        //Keep a histogram of fret usage
        fretPicked = (fretPicked+12*ctx->fret->used) % ctx->fret->used;
        //for(int f=0; f < ctx->fretsUsed; f++)
        //{
        //    ctx->fretUsage[f] *= (1.0*(ctx->fretsUsed-1))/ctx->fretsUsed;
        //}
        ctx->fret->usage[ fretPicked ] += 0.25;
        
    }
    
    float targetDrift = (ctx->fingers[finger].endPitch - thisPitch);
    if( isMoving )
    {
        float tuneRate = ctx->tuneSpeed;// * fabs(targetDrift);
        
        ctx->pitchDiffByFinger[finger] = (1 - tuneRate) * ctx->pitchDiffByFinger[finger] + tuneRate * targetDrift;                
    }
    else
    {
        if(ctx->initSnap)
        {
            ctx->pitchDiffByFinger[finger] = targetDrift;                    
        }
        else
        {
            ctx->pitchDiffByFinger[finger] = -0.5;
        }
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
    for(int f=0; f < ctx->fret->used; f++)
    {
        ctx->fret->usage[f] *= (1.0*(ctx->fret->used-1))/ctx->fret->used;
    }
    //ctx->fretUsage[ fretPicked ] += 0.25;    
}



void PitchHandler_getFretsBegin(struct PitchHandler_context* ctx)
{
    ctx->fretiterator = -ctx->fret->used;
    ctx->fretoffsetY = ctx->fretoffsetYInitial;
    ctx->fretoffsetX = ctx->fretoffsetXInitial;
}

int PitchHandler_getFret(struct PitchHandler_context* ctx, float* pitch,float* x,float* y,int* importance,float* usage,int* fretval)
{    
    struct Fret_context* fctx = ctx->fret;
    if(fctx->used == 0)return 0;
    
    float pitchVal = Fret_getPitchFromFret(fctx,ctx->fretiterator) - ctx->noteDiff;
    
    //If we have gone past the right edge of the screen
    if(pitchVal + ctx->fretoffsetX > ctx->colCount)
    {
        //and shift the string while moving to the next
        ctx->fretoffsetX -= ctx->tuneInterval[(int)ctx->fretoffsetY];
        ctx->fretoffsetY += 1;
        
        //go down an octave
        ctx->fretiterator -= fctx->used;
        pitchVal -= 12;
        //ctx->fretOffsetX -= 12;
    }
    
    *pitch = pitchVal;
    *x = (pitchVal + ctx->fretoffsetX) / ctx->colCount;
    *y = (ctx->fretoffsetY) / ctx->rowCount;
    int ourFret = (ctx->fretiterator + 12*fctx->used) % fctx->used;
    *importance = fctx->importance[ourFret];
    *usage = fctx->usage[ourFret];
    *fretval = ourFret;
    ctx->fretiterator++;
    return *y < 1;
}

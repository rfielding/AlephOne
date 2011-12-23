//
//  Fret.c
//  AlephOne
//
//  Created by Robert Fielding on 12/18/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "Fret.h"
#include <math.h>


struct Fret_context* Fret_init(void* (*allocFn)(unsigned long))
{
    struct Fret_context* ctx = 
        (struct Fret_context*)allocFn(sizeof(struct Fret_context));
    ctx->used = 0;

    for(int i=0; i<16; i++)
    {
        ctx->usage[i] = 0;
    }
    return ctx;
}

//Moveable fret generator
void Fret_clearFrets(struct Fret_context* ctx)
{
    ctx->used=0;
}


void Fret_placeFret(struct Fret_context* ctx, float pitch, int importance)
{
    //Must be in range 0..12
    pitch = fmod(pitch,12);
    
    //Don't re-add existing values, but allow importance re-assign
    for(int f=0; f < ctx->used; f++)
    {
        if(pitch == ctx->cents[f])
        {
            //Don't bother adding it, as it's already in here
            //But you can re-assign the importance
            ctx->importance[f] = importance;
            return;
        }
    }
    
    ctx->cents[ctx->used] = pitch;
    ctx->importance[ctx->used] = importance;
    
    int thisFret = ctx->used;
    while(thisFret > 0 && ctx->cents[thisFret] < ctx->cents[thisFret-1])
    {
        //Swap and drop down one.  Bubble sort!
        float p = ctx->cents[thisFret];
        int i = ctx->importance[thisFret];
        ctx->cents[thisFret] = ctx->cents[thisFret-1];
        ctx->importance[thisFret] = ctx->importance[thisFret-1];
        ctx->cents[thisFret-1] = p;
        ctx->importance[thisFret-1] = i;
        thisFret--;
    }
    for(int f=0; f < ctx->used; f++)
    {
        ctx->usage[f] = 1.0/ctx->importance[f];
    }
    ctx->used++;
}



float Fret_getPitchFromFret(struct Fret_context* ctx, int fret)
{
    int octave = (int)floorf(1.0 * fret / ctx->used);
    return 12.0 * octave + ctx->cents[(fret+12*ctx->used) % ctx->used];
}

/**
 * TODO: this is inappropriate if we have a large number of frets per octave.
 * It should be a binary search in that case.
 */
float Fret_getTarget(struct Fret_context* ctx, float pitch, int* fretP)
{
    if(ctx->used == 0)return pitch;
    
    int octaveEst = ctx->used*floorf(pitch / 12.0);
    float pitchVal = pitch;
    float bestDistance=48;
    int fret;
    int bestFret;
    for(fret=octaveEst - ctx->used; fret <= octaveEst + ctx->used; fret++)
    {
        float p = Fret_getPitchFromFret(ctx,fret);
        float dist = fabs(pitch - 0.5 - p);
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

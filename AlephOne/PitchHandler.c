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
//static float tuneInterval = 5; //4.9804499913461244;  //12*log2f(4.0/3); //is Just intonation btw
static float tuneSpeed = 0.1;
static float rowCount = 3;
static float colCount = 5;
static int noteDiff = (12*4); //(48-1);

static float frets[1024];
static int fretsUsed=0;
static int fretIterator=0;
static float fretOffsetY=0;
static float fretOffsetX=0;
static float fretOffsetYInitial=0.5;
static float fretOffsetXInitial=0.5;


static int   lastFingerDown = NOBODY;
static float lastNoteDown = 0;
static int   noteDiffOurs = 0;
static float   noteDiffByFinger[FINGERMAX];
static float   pitchDiffByFinger[FINGERMAX];
static float   tuneInterval[FINGERMAX];

static int doOctaveRounding=1;


struct FingerInfo fingers[FINGERMAX];

struct FingerInfo* PitchHandler_fingerState(int finger)
{
    return &fingers[finger];
}

int PitchHandler_getOctaveRounding()
{
    return doOctaveRounding;
}

void PitchHandler_setOctaveRounding(int octRound)
{
    doOctaveRounding = octRound;
}

float PitchHandler_getTuneInterval(int string)
{
    return tuneInterval[string];
}

void PitchHandler_setTuneInterval(int string,float tuning)
{
    tuneInterval[string] = tuning;
}

float PitchHandler_getTuneSpeed()
{
    return tuneSpeed;
}

void PitchHandler_setTuneSpeed(float tuneSpeedArg)
{
    tuneSpeed = tuneSpeedArg;
}

float PitchHandler_getRowCount()
{
    return rowCount;
}

void PitchHandler_setRowCount(float rowCountArg)
{
    rowCount = rowCountArg;
}
        
float PitchHandler_getColCount()
{
    return colCount;
}

void PitchHandler_setColCount(float colCountArg)
{
    colCount = colCountArg;
}

int PitchHandler_getNoteDiff()
{
    return noteDiff;
}

void PitchHandler_setNoteDiff(int noteDiffArg)
{
    noteDiff = noteDiffArg;
}

void PitchHandler_unpickPitch(int finger)
{
    fingers[finger].isActive = 0;
}

struct FingerInfo* PitchHandler_pickPitch(int finger,int isMoving,float x,float y)
{
    fingers[finger].string = (rowCount * y);
    fingers[finger].expr = (rowCount*y) - fingers[finger].string;
    float fret = colCount*x;
    fingers[finger].pitchRaw = (fret + tuneInterval[fingers[finger].string]); 
    
    fingers[finger].fingerX = x;
    fingers[finger].fingerY = y;
    fingers[finger].isActive = 1;    
    
    float thisPitch = fingers[finger].pitchRaw;
    

    
    if( isMoving )
    {
        noteDiffOurs = noteDiffByFinger[finger];
    }
    else
    {
        lastFingerDown = finger;
        noteDiffOurs = noteDiff;
        noteDiffByFinger[finger] = noteDiff;
    }

    
    thisPitch += noteDiffOurs;
    fingers[finger].beginPitch = thisPitch;
    fingers[finger].endPitch = PitchHandler_getTarget(thisPitch);
    
    float targetDrift = (fingers[finger].endPitch - thisPitch);
    if( isMoving )
    {
        float pitchDiff = fabs(fingers[finger].beginPitch - fingers[finger].endPitch);
        float tuneRate = tuneSpeed * pitchDiff;
        
        pitchDiffByFinger[finger] = (1 - tuneRate) * pitchDiffByFinger[finger] + tuneRate * targetDrift;                
    }
    else
    {
        pitchDiffByFinger[finger] = targetDrift;        
    }
    thisPitch += pitchDiffByFinger[finger];
    
    if(finger == lastFingerDown)
    {
        float diff = (thisPitch - lastNoteDown);
        if(doOctaveRounding)
        {
            if(diff > 6.5)
            {
                thisPitch -= 12;
                noteDiff -= 12;
                noteDiffOurs -= 12;
            }
            if(diff <= -6.5)
            {
                thisPitch += 12;
                noteDiff += 12;
                noteDiffOurs += 12;
            }            
        }
        while(thisPitch < -0.5)
        {
            thisPitch += 12;
            noteDiff += 12;
            noteDiffOurs += 12;
        }
        while(thisPitch >= 127.5)
        {
            thisPitch -= 12;
            noteDiff -= 12;
            noteDiffOurs -= 12;
        }
        lastNoteDown = thisPitch;
    }
    noteDiffByFinger[finger] = noteDiffOurs;        

    
    fingers[finger].pitchX = fingers[finger].fingerX + (pitchDiffByFinger[finger] + 0.5)/colCount;
    fingers[finger].pitchY = fingers[finger].fingerY;
    fingers[finger].pitch = thisPitch;
    
    
    return &fingers[finger];
}


//Moveable fret generator
void PitchHandler_clearFrets()
{
    fretsUsed=0;
}

//This must span an octave from 0 <= pitch < 12.0, or everything breaks
void PitchHandler_placeFret(float pitch)
{
    frets[fretsUsed] = pitch;
    fretsUsed++;
}

void PitchHandler_getFretsBegin()
{
    fretIterator=-fretsUsed;
    fretOffsetY=fretOffsetYInitial;
    fretOffsetX=fretOffsetXInitial;
}

float PitchHandler_getPitchFromFret(int fret)
{
    int octave = (int)floorf(1.0 * fret / fretsUsed);
    return 12.0 * octave + frets[(fret+10*fretsUsed) % fretsUsed];
}

/**
 * TODO: this is inappropriate if we have a large number of frets per octave.
 * It should be a binary search in that case.
 */
float PitchHandler_getTarget(float pitch)
{
    int octaveEst = fretsUsed*floorf(pitch / 12.0);
    float pitchVal = pitch;
    float bestDistance=48;
    for(int fret=octaveEst-fretsUsed; fret <= octaveEst+fretsUsed; fret++)
    {
        float p = PitchHandler_getPitchFromFret(fret);
        float dist = fabs(pitch - fretOffsetXInitial - p);
        if(dist < bestDistance)
        {
            bestDistance = dist;
            pitchVal = p;
        }
    }
    return pitchVal;
}

int PitchHandler_getFret(float* pitch,float* x,float* y)
{
    float pitchVal = PitchHandler_getPitchFromFret(fretIterator)-noteDiff;
    
    if(pitchVal+fretOffsetX > colCount)
    {
        //Move back an octave in addition to the tuning interval
        //and move back an octave number of frets
        fretOffsetX -= tuneInterval[(int)fretOffsetY] + 12;
        //We went off the right edge of the screen
        //so move up a string
        fretOffsetY+=1;
        fretIterator -= fretsUsed;
        //notice that we ASSUME that the screen doesn't get wider than an octave here!
    }
    
    *pitch = pitchVal;
    *x = (pitchVal+fretOffsetX)/colCount;
    *y = (fretOffsetY)/rowCount;
    
    fretIterator++;
    return (fretOffsetY < rowCount+1);
}



//
//  PitchHandler.c
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "PitchHandler.h"
#include <math.h>

//Quick oct rounding hack
float PitchHandler_pickPitch(int finger,int isMoving,float x,float y,int* stringP,float* exprP)
{
    static int   lastFingerDown = -1;
    static float lastNoteDown = 0;
    static int   octDiff = 48;
    static int   octDiffOurs = -1;
    static int   octDiffByFinger[16];
    static float   yDiffByFinger[16];
    float tuning = 12*log2f(4.0/3); //Just fourths (rather than simply diatonic fourth)
    
    if( isMoving )
    {
        octDiffOurs = octDiffByFinger[finger];
    }
    else
    {
        lastFingerDown = finger;
        octDiffOurs = octDiff;
        octDiffByFinger[finger] = octDiff;
    }
    
    *stringP = (3.0 * x);
    *exprP = (3.0*x) - *stringP;
    float fret = 5*y;
    float thisPitch = (fret + (*stringP)*tuning + octDiffOurs);  
    float targetPitch = (int)thisPitch;
    float targetDrift = (targetPitch - thisPitch);
    if( isMoving )
    {
        yDiffByFinger[finger] = 0.9f * yDiffByFinger[finger] + 0.1f * targetDrift;                
    }
    else
    {
        yDiffByFinger[finger] = targetDrift;        
    }
    thisPitch += yDiffByFinger[finger];
    
    if(finger == lastFingerDown)
    {
        float diff = (thisPitch - lastNoteDown);
        if(diff > 6.5)
        {
            thisPitch -= 12;
            octDiff -= 12;
            octDiffOurs -= 12;
        }
        if(diff <= -6.5)
        {
            thisPitch += 12;
            octDiff += 12;
            octDiffOurs += 12;
        }
        while(thisPitch < -0.5)
        {
            thisPitch += 12;
            octDiff += 12;
            octDiffOurs += 12;
        }
        while(thisPitch >= 127.5)
        {
            thisPitch -= 12;
            octDiff -= 12;
            octDiffOurs -= 12;
        }
        lastNoteDown = thisPitch;
    }
    octDiffByFinger[finger] = octDiffOurs;        
    return thisPitch;
}


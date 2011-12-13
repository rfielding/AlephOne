//
//  PitchHandler.h
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


/**
 * All coordinates are in the rectangle ((0,0),(1,1)) at this point, 
 * so that there are no funny transformations required throughout the code.
 *
 * Transforms module changes from the system specific space into this one.
 * It is important that it match this because the rendering is expected to use it
 * as well.
 */

struct FingerInfo
{
    float fingerX;
    float fingerY;
    float pitchX;
    float pitchY;
    float targetX;
    float targetY;
    int isActive;
    int string;
    float expr;
    float pitchRaw;
    float pitch;
    float beginPitch;
    float endPitch;
};


//With x,y that came out of PitchHandler_translate,
struct FingerInfo* PitchHandler_pickPitchRaw(int finger,float x,float y);

struct FingerInfo* PitchHandler_fingerState(int finger);

//Tuning between strings
float PitchHandler_getTuneInterval(int string);
void PitchHandler_setTuneInterval(int string,float tuning);

//The number of strings
float PitchHandler_getRowCount();
void PitchHandler_setRowCount(float rowCount);

//The number of frets in 12ET
float PitchHandler_getColCount();
void PitchHandler_setColCount(float colCount);

int PitchHandler_getOctaveRounding();
void PitchHandler_setOctaveRounding(int octRound);


//Moveable fret generator
void PitchHandler_clearFrets();
void PitchHandler_placeFret(float pitch);
void PitchHandler_getFretsBegin();
int PitchHandler_getFret(float* pitch,float* x,float* y);
float PitchHandler_getTarget(float pitch);

//Given where the finger is (in pitch terms), and whether it's moving (or just begin),
//Compute the adjusted pitch for where this finger is beginning from, the end pitch it wants to go to.
//The returned pitch is somewhere in between beginPitch and endPitch
struct FingerInfo* PitchHandler_pickPitch(int finger,int isMoving,float x,float y);
void PitchHandler_unpickPitch(int finger);

//The rate at which we drift to endPitch
float PitchHandler_getTuneSpeed();
void PitchHandler_setTuneSpeed(float tuneSpeed);

//Move interface 12ET notes
int PitchHandler_getNoteDiff();
void PitchHandler_setNoteDiff(int noteDiff);


//
//  PitchHandler.h
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

//Translate from ((0,0),(1,1)) rectangle space into the internal coordinate space
void PitchHandler_translate(float* xp,float* yp);
void PitchHandler_getOrientation(float* matrix);
void PitchHandler_clockwiseOrientation();
void PitchHandler_xflipOrientation();


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
float PitchHandler_getTuneInterval();
void PitchHandler_setTuneInterval(float tuning);

//The number of strings
float PitchHandler_getRowCount();
void PitchHandler_setRowCount(float rowCount);

//The number of frets in 12ET
float PitchHandler_getColCount();
void PitchHandler_setColCount(float colCount);




//Given where the finger is (in pitch terms), and whether it's moving (or just begin),
//Compute the adjusted pitch for where this finger is beginning from, the end pitch it wants to go to.
//The returned pitch is somewhere in between beginPitch and endPitch
struct FingerInfo* PitchHandler_pickPitch(int finger,int isMoving,float x,float y);
void PitchHandler_unpickPitch(int finger);

//The rate at which we drift to endPitch
float PitchHandler_getTuneSpeed();
void PitchHandler_setTuneSpeed(float tuneSpeed);

//Transpose in 12ET notes
float PitchHandler_getNoteDiff();
void PitchHandler_setNoteDiff(float noteDiff);


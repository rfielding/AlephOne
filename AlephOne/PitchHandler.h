//
//  PitchHandler.h
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


//Get the pitch that the finger is on if the play surface is snapped to a rectangle around [(0,0), (1,1)]
//Also, compute which string we are on, and how much expression is in it
//
//The pitch returned is where the finger *is*
float PitchHandler_pickPitchRaw(int finger,float x,float y,int* stringP,float* exprP);

//Given where the finger is (in pitch terms), and whether it's moving (or just begin),
//Compute the adjusted pitch for where this finger is beginning from, the end pitch it wants to go to.
//The returned pitch is somewhere in between beginPitch and endPitch
float PitchHandler_pickPitch(int finger,int isMoving, float thisPitch, float* beginPitchP, float* endPitchP);


//Some getters and setters for controls, so we can hook up controls into the UI later
float PitchHandler_getTuneInterval();
void PitchHandler_setTuneInterval(float tuning);

float PitchHandler_getTuneSpeed();
void PitchHandler_setTuneSpeed(float tuneSpeed);

float PitchHandler_getRowCount();
void PitchHandler_setRowCount(float rowCount);

float PitchHandler_getColCount();
void PitchHandler_setColCount(float colCount);

float PitchHandler_getNoteDiff();
void PitchHandler_setNoteDiff(float noteDiff);

//Get a 4x4 matrix that describes our orientation
void PitchHandler_getOrientation(float* matrix);
void PitchHandler_clockwiseOrientation();
void PitchHandler_xflipOrientation();
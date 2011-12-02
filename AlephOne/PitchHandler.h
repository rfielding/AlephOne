//
//  PitchHandler.h
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


float PitchHandler_pickPitchRaw(int finger,float x,float y,int* stringP,float* exprP);
float PitchHandler_pickPitch(int finger,int isMoving, float thisPitch, float* beginPitchP, float* endPitchP);

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

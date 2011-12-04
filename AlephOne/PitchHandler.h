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



//With x,y that came out of PitchHandler_translate,
float PitchHandler_pickPitchRaw(int finger,float x,float y,int* stringP,float* exprP);

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
float PitchHandler_pickPitch(int finger,int isMoving, float thisPitch, float* beginPitchP, float* endPitchP);

//The rate at which we drift to endPitch
float PitchHandler_getTuneSpeed();
void PitchHandler_setTuneSpeed(float tuneSpeed);

//Transpose in 12ET notes
float PitchHandler_getNoteDiff();
void PitchHandler_setNoteDiff(float noteDiff);


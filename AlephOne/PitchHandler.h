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

struct PitchHandlerContext;

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

struct PitchHandlerContext* PitchHandler_init(void* (*allocFn)(unsigned long));

//With x,y that came out of PitchHandler_translate,
struct FingerInfo* PitchHandler_pickPitchRaw(struct PitchHandlerContext* ctx, int finger,float x,float y);

struct FingerInfo* PitchHandler_fingerState(struct PitchHandlerContext* ctx, int finger);

//Tuning between strings
float PitchHandler_getTuneInterval(struct PitchHandlerContext* ctx, int string);
void PitchHandler_setTuneInterval(struct PitchHandlerContext* ctx, int string,float tuning);

//The number of strings
float PitchHandler_getRowCount(struct PitchHandlerContext* ctx);
void PitchHandler_setRowCount(struct PitchHandlerContext* ctx, float rowCount);

//The number of frets in 12ET
float PitchHandler_getColCount(struct PitchHandlerContext* ctx);
void PitchHandler_setColCount(struct PitchHandlerContext* ctx, float colCount);

int PitchHandler_getOctaveRounding(struct PitchHandlerContext* ctx);
void PitchHandler_setOctaveRounding(struct PitchHandlerContext* ctx, int octRound);

//Given where the finger is (in pitch terms), and whether it's moving (or just begin),
//Compute the adjusted pitch for where this finger is beginning from, the end pitch it wants to go to.
//The returned pitch is somewhere in between beginPitch and endPitch
struct FingerInfo* PitchHandler_pickPitch(struct PitchHandlerContext* ctx, int finger,int isMoving,float x,float y);
void PitchHandler_unpickPitch(struct PitchHandlerContext* ctx, int finger);

//The rate at which we drift to endPitch
float PitchHandler_getTuneSpeed(struct PitchHandlerContext* ctx);
void PitchHandler_setTuneSpeed(struct PitchHandlerContext* ctx, float tuneSpeed);

//Move interface 12ET notes
int PitchHandler_getNoteDiff(struct PitchHandlerContext* ctx);
void PitchHandler_setNoteDiff(struct PitchHandlerContext* ctx, int noteDiff);




//Moveable fret generator - this might be broken out into its own class
void PitchHandler_clearFrets(struct PitchHandlerContext* ctx);
void PitchHandler_placeFret(struct PitchHandlerContext* ctx, float pitch,int importance);
void PitchHandler_getFretsBegin(struct PitchHandlerContext* ctx);
int PitchHandler_getFret(struct PitchHandlerContext* ctx, float* pitch,float* x,float* y,int* importance);
float PitchHandler_getTarget(struct PitchHandlerContext* ctx, float pitch);


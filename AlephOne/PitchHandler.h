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

struct PitchHandler_context;

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

struct PitchHandler_context* PitchHandler_init(void* (*allocFn)(unsigned long),int (*fail)(const char*,...),int (*logger)(const char*,...));

//With x,y that came out of PitchHandler_translate,
struct FingerInfo* PitchHandler_pickPitchRaw(struct PitchHandler_context* ctx, int finger,float x,float y);

struct FingerInfo* PitchHandler_fingerState(struct PitchHandler_context* ctx, int finger);

//Tuning between strings
float PitchHandler_getTuneInterval(struct PitchHandler_context* ctx, int string);
void PitchHandler_setTuneInterval(struct PitchHandler_context* ctx, int string,float tuning);

//The number of strings
float PitchHandler_getRowCount(struct PitchHandler_context* ctx);
void PitchHandler_setRowCount(struct PitchHandler_context* ctx, float rowCount);

//The number of frets in 12ET
float PitchHandler_getColCount(struct PitchHandler_context* ctx);
void PitchHandler_setColCount(struct PitchHandler_context* ctx, float colCount);

int PitchHandler_getOctaveRounding(struct PitchHandler_context* ctx);
void PitchHandler_setOctaveRounding(struct PitchHandler_context* ctx, int octRound);

//Given where the finger is (in pitch terms), and whether it's moving (or just begin),
//Compute the adjusted pitch for where this finger is beginning from, the end pitch it wants to go to.
//The returned pitch is somewhere in between beginPitch and endPitch
struct FingerInfo* PitchHandler_pickPitch(struct PitchHandler_context* ctx, int finger,int isMoving,float x,float y);
void PitchHandler_unpickPitch(struct PitchHandler_context* ctx, int finger);

//The rate at which we drift to endPitch
float PitchHandler_getTuneSpeed(struct PitchHandler_context* ctx);
void PitchHandler_setTuneSpeed(struct PitchHandler_context* ctx, float tuneSpeed);

//Move interface 12ET notes
int PitchHandler_getNoteDiff(struct PitchHandler_context* ctx);
void PitchHandler_setNoteDiff(struct PitchHandler_context* ctx, int noteDiff);



//Get rid of existing frets
void PitchHandler_clearFrets(struct PitchHandler_context* ctx);

//Frets can be placed in any order
void PitchHandler_placeFret(struct PitchHandler_context* ctx, float pitch,int importance);


void PitchHandler_getFretsBegin(struct PitchHandler_context* ctx);
int PitchHandler_getFret(struct PitchHandler_context* ctx, float* pitch,float* x,float* y,int* importance,float* usage);

//Given a pitch, find the target that it wants to snap to (given the frets in use)
float PitchHandler_getTarget(struct PitchHandler_context* ctx, float pitch,int* fretP);

void PitchHandler_tick(struct PitchHandler_context * ctx);


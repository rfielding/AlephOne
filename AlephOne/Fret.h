//
//  Fret.h
//  AlephOne
//
//  Created by Robert Fielding on 12/18/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

/**
 This code is the basis for making moveable frets.  It stays out of the business
 of how the screen is laid out and just sticks to pitches and away from implying any
 (x,y) coordinates.  This should allow this code to remain relevant for piano-like layouts.
 */
#define FRETMAX 1024

struct Fret_context
{
    float cents[FRETMAX];
    int   importance[FRETMAX];
    float usage[FRETMAX];
    int used;
};

struct Fret_context* Fret_init(void* (*allocFn)(unsigned long));

//Get rid of existing frets
void Fret_clearFrets(struct Fret_context* ctx);

//Frets can be placed in any order
void Fret_placeFret(struct Fret_context* ctx, float pitch,int importance);

//Get a pitch given the fret
float Fret_getPitchFromFret(struct Fret_context* ctx, int fret);

//Given a pitch, find the target that it wants to snap to (given the frets in use)
float Fret_getTarget(struct Fret_context* ctx, float pitch,int* fretP);
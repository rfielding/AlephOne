//
//  Parameters.c
//  AlephOne
//
//  Created by Robert Fielding on 4/15/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//

static float distortion = 0.1;
static float reverb = 1;
static float timbre = 1;
static float detune = 1;

float getDistortion()
{
    return distortion;
}

void setDistortion(float val)
{
    distortion = val;
}

float getReverb()
{
    return reverb;
}

void setReverb(float val)
{
    reverb = val;
}

float getTimbre()
{
    return timbre;
}

void setTimbre(float val)
{
    timbre = val;
}

float getDetune()
{
    return detune;
}

void setDetune(float val)
{
    detune = val;
}
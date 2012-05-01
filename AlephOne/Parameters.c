//
//  Parameters.c
//  AlephOne
//
//  Created by Robert Fielding on 4/15/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//

static float distortion = 0.9;
static float reverb = 0.9;
static float timbre = 0.9;
static float detune = 0.75;
static float sensitivity = 1;

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

float getSensitivity()
{
    return sensitivity;
}

void setSensitivity(float val)
{
    sensitivity = val;
}
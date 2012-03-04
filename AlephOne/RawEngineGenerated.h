//
//  RawEngineGenerated.h
//  AlephOne
//
//  Created by Robert Fielding on 3/4/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//

#define WAVEMAX (1024*4)
#define SAMPLESMAX 1024
#define DIST 2
#define EXPR 2

float  waveMix        [EXPR][DIST][WAVEMAX];
float _waveFundamental            [WAVEMAX];
float sampleIndexArray[SAMPLESMAX];

float renderNoiseInnerLoopInParallel(float* output,float notep,float detune,float pitchLocation,float p,
                                     unsigned long samples,float invSamples,float currentVolume,float deltaVolume,float currentExpr,float deltaExpr);

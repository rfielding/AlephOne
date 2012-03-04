//
//  RawEngineGenerated.c
//  AlephOne
//
//  Created by Robert Fielding on 3/4/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//
#import <Accelerate/Accelerate.h>
#import "RawEngineGenerated.h"

#define SAMPLEINPARALLEL(samples,statement) for(int i=0; i<samples; i++) { statement; }

static inline void xDSP_vcp(float* src,float* dst,int count)
{
    memcpy(dst,src,count*sizeof(float));
}


float cyclesArray[SAMPLESMAX];
float cyclesIntArray[SAMPLESMAX];
float cyclesLocation[SAMPLESMAX];
int   jLocation[SAMPLESMAX];
float vArray[SAMPLESMAX];
float eArray[SAMPLESMAX];
float eNotArray[SAMPLESMAX];
float dArray[SAMPLESMAX];
float dNotArray[SAMPLESMAX];
float unSquishedPartArray[SAMPLESMAX];
float unSquishedTotalArray[SAMPLESMAX];
float fundamentalArray[SAMPLESMAX];
float waveMixArray[SAMPLESMAX];

static inline void renderNoiseComputeWaveIndexJ(float phase,float cyclesPerSample,unsigned long samples)
{
    //
    //cycles[i]         = i * cyclesPerSample + phase
    //
    
    xDSP_vcp(sampleIndexArray,cyclesArray,samples); 
    vDSP_vsmul(cyclesArray,1,&cyclesPerSample,cyclesArray,1,samples);
    vDSP_vsadd(cyclesArray,1,&phase,cyclesArray,1,samples);
    
    //
    //jLocation[i]      = (int)((cycles[i] - (int)cycles[i]) * WAVEMAX)
    //
    
    SAMPLEINPARALLEL(samples, cyclesIntArray[i]  = (int)cyclesArray[i]);
    xDSP_vcp(cyclesArray,cyclesLocation,samples);
    float negone=-1;
    vDSP_vsmul(cyclesIntArray,1,&negone,cyclesIntArray,1,samples);
    vDSP_vadd(cyclesLocation,1,cyclesIntArray,1,cyclesLocation,1,samples);
    
    float wavemax=WAVEMAX;
    vDSP_vsmul(cyclesLocation,1,&wavemax,cyclesLocation,1,samples);
    
    SAMPLEINPARALLEL(samples, jLocation[i]       = (int)cyclesLocation[i]);    
}

static inline void renderNoiseComputeV(float currentVolume, float deltaVolume, unsigned long samples)
{
    //
    // deltaVolume = invSamples * diffVolume
    // v[i] = (i * (invSamples * diffVolume)) + currentVolume
    //
    
    xDSP_vcp(sampleIndexArray,vArray,samples);    
    vDSP_vsmul(vArray,1,&deltaVolume,vArray,1,samples);
    vDSP_vsadd(vArray,1,&currentVolume,vArray,1,samples);        
}

static inline void renderNoiseComputeE(float currentExpr, float deltaExpr, unsigned long samples)
{
    float one=1;
    
    //
    // deltaExpr = invSamples * diffExpr
    // e[i] = (i * (invSamples * diffExpr)) + currentExpr
    // eNot[i] = (1-e[i])
    //
    
    xDSP_vcp(sampleIndexArray,eArray,samples);    
    vDSP_vsmul(eArray,1,&deltaExpr,eArray,1,samples);
    vDSP_vsmul(eArray,1,&currentExpr,eArray,1,samples);      
    vDSP_vfill(&one,eNotArray,1,samples);
    vDSP_vsub(eNotArray,1,eArray,1,eNotArray,1,samples);    
    
    //
    // d[i]    = eNot[i] * v[i] 
    // dNot[i] = (1-d[i])
    //
    xDSP_vcp(eNotArray,dArray,samples);
    vDSP_vmul(dArray,1, vArray,1, dArray,1, samples);
    vDSP_vfill(&one,dNotArray,1,samples);
    vDSP_vsub(dNotArray,1, dArray,1, dNotArray,1, samples);    
}

/**
 Good God!  This is assembly language.
 */
float renderNoiseInnerLoopInParallel(float* output,float notep,float detune,float pitchLocation,float p,
                                                   unsigned long samples,float invSamples,float currentVolume,float deltaVolume,float currentExpr,float deltaExpr)
{
    float cyclesPerSample = powf(2,(notep-33+(1-currentExpr)*detune*(1-pitchLocation))/12) * (440/(44100.0 * 32));
    
    renderNoiseComputeWaveIndexJ(p,cyclesPerSample, samples);
    renderNoiseComputeV(currentVolume, deltaVolume, samples);    
    renderNoiseComputeE(currentExpr, deltaExpr, samples);
    
    xDSP_vcp(dArray,unSquishedPartArray,samples);
    
    SAMPLEINPARALLEL(samples, unSquishedPartArray[i]  *= waveMix[0][1][jLocation[i]]);
    xDSP_vcp(unSquishedPartArray,unSquishedTotalArray,samples);
    
    xDSP_vcp(dNotArray,unSquishedPartArray,samples);
    
    SAMPLEINPARALLEL(samples, unSquishedPartArray[i]  *= waveMix[0][0][jLocation[i]]);
    vDSP_vadd(unSquishedTotalArray,1, unSquishedPartArray,1, unSquishedTotalArray,1, samples);
    
    vDSP_vmul(eNotArray,1, unSquishedTotalArray,1, eNotArray,1, samples);
    xDSP_vcp(dArray,unSquishedPartArray,samples);
    SAMPLEINPARALLEL(samples, unSquishedPartArray[i]  *= waveMix[1][1][jLocation[i]]);
    xDSP_vcp(unSquishedPartArray,unSquishedTotalArray, samples);
    
    xDSP_vcp(dNotArray, unSquishedPartArray, samples);
    
    SAMPLEINPARALLEL(samples, unSquishedPartArray[i]  *= waveMix[1][0][jLocation[i]]);
    vDSP_vadd(unSquishedTotalArray,1, unSquishedPartArray,1, unSquishedTotalArray,1, samples);    
    vDSP_vmul(eArray,1, unSquishedTotalArray,1, eArray,1, samples);
    vDSP_vadd(unSquishedTotalArray,1, eNotArray,1, unSquishedTotalArray,1, samples);
    
    float pitchLocationNot=(1-pitchLocation);
    vDSP_vsmul(unSquishedTotalArray,1,&pitchLocationNot,unSquishedTotalArray,1,samples);
    
    SAMPLEINPARALLEL(samples, fundamentalArray[i]  = _waveFundamental[jLocation[i]]);
    vDSP_vsmul(fundamentalArray,1,&pitchLocation,fundamentalArray,1,samples);
    vDSP_vadd(fundamentalArray,1, unSquishedPartArray,1, unSquishedPartArray,1, samples);
    vDSP_vmul(unSquishedTotalArray,1, vArray,1, unSquishedTotalArray,1, samples);
    
    vDSP_vadd(output,1, unSquishedTotalArray,1, output,1, samples);
    return (cyclesPerSample*samples) + p;
}

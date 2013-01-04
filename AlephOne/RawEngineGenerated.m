//
//  RawEngineGenerated.c
//  AlephOne
//
//  Created by Robert Fielding on 3/4/12.
//  Copyright (c) 2012 Rob Fielding Software.
//
#import <Accelerate/Accelerate.h>
#import "RawEngineGenerated.h"

/*
static inline void xDSP_vcp(float* src,float* dst,int count)
{
    memcpy(dst,src,count*sizeof(float));
}


unsigned int jLocation[SAMPLESMAX] __attribute__ ((aligned));
float vArray[SAMPLESMAX] __attribute__ ((aligned));
float eArray[SAMPLESMAX] __attribute__ ((aligned));
float eNotArray[SAMPLESMAX] __attribute__ ((aligned));
float dArray[SAMPLESMAX] __attribute__ ((aligned));
float dNotArray[SAMPLESMAX] __attribute__ ((aligned));
float registerLeft[SAMPLESMAX] __attribute__ ((aligned));
float registerRight[SAMPLESMAX] __attribute__ ((aligned));

float waveIndexArray[SAMPLESMAX] __attribute__ ((aligned));
float waveMixArray[SAMPLESMAX] __attribute__ ((aligned));
float waveOct[SAMPLESMAX] __attribute__ ((aligned));
float waveOct2[SAMPLESMAX] __attribute__ ((aligned));

float timbreArray[SAMPLESMAX] __attribute__((aligned));

float wavemax=WAVEMAX;

static inline void renderNoiseComputeWaveIndexJ(float phase,float cyclesPerSample,unsigned long samples)
{
    //
    //cycles[i]         = i * cyclesPerSample + phase
    //
    
    //xDSP_vcp(sampleIndexArray,registerLeft,samples); 
    //vDSP_vsmul(registerLeft,1,&cyclesPerSample,registerLeft,1,samples);
    float zero = 0;
    //float fsamples = samples;
    vDSP_vramp(&zero,&cyclesPerSample,registerLeft,1,samples);
    vDSP_vsadd(registerLeft,1,&phase,registerLeft,1,samples);
    
    //
    //waveIndexArray[i]      = (frac(cycles[i]) * WAVEMAX)
    //
    
    vDSP_vfrac(registerLeft,1,registerLeft,1,samples);
    vDSP_vsmul(registerLeft,1,&wavemax,waveIndexArray,1,samples);    
}

static inline void renderNoiseComputeV(float currentVolume, float deltaVolume, unsigned long samples)
{
    //
    // deltaVolume = invSamples * diffVolume
    // v[i] = (i * (invSamples * diffVolume)) + currentVolume
    //
    vDSP_vramp(&currentVolume,&deltaVolume,vArray,1,samples);
    //printf("%f %f %f\n",vArray[0],vArray[samples-1],deltaVolume);
}

static inline void renderNoiseComputeE(float currentExpr, float deltaExpr, unsigned long samples)
{
    float one=1;
    //
    // deltaExpr = invSamples * diffExpr
    // e[i] = (i * (invSamples * diffExpr)) + currentExpr
    // //e[i] = bound{0,1}(2*(e[0] - 0.25))
    // eNot[i] = (1-e[i])
    //
    vDSP_vramp(&currentExpr,&deltaExpr,eArray,1,samples);
    vDSP_vfill(&one,eNotArray,1,samples);
    vDSP_vsub(eNotArray,1,eArray,1,eNotArray,1,samples);    
    
    //
    // d[i]    = eNot[i] * v[i] 
    // dNot[i] = (1-d[i])
    //
    vDSP_vmul(eNotArray,1, vArray,1, dArray,1, samples);
    vDSP_vfill(&one,dNotArray,1,samples);
    vDSP_vsub(dNotArray,1, dArray,1, dNotArray,1, samples);    
}

static inline void renderNoiseSampleMixInternal(float notep,int eL,int dL,int eH,int dH, float* eScaleArray,unsigned long samples)
{
    float foct = (notep/12);
    if(foct+2>OCTAVES)foct=OCTAVES-2;
    int oct = (int)foct;
    float o = foct - oct;
    float o2 = 1 - o;
    
    float* waveLo = waveMix[oct+1][eL][dL];
    float* waveLo2 = waveMix[oct][eL][dL];
    
    float* waveHi = waveMix[oct+1][eH][dH];
    float* waveHi2 = waveMix[oct][eH][dH];
    
    
    vDSP_vindex(waveHi,waveIndexArray,1,waveOct,1,samples);
    vDSP_vsmul(waveOct,1,&o,waveOct,1,samples);
    vDSP_vindex(waveHi2,waveIndexArray,1,waveOct2,1,samples);
    vDSP_vsmul(waveOct2,1,&o2,waveOct2,1,samples);
    vDSP_vadd(waveOct,1,waveOct2,1,waveOct,1,samples);
    vDSP_vmul(waveOct,1,dArray,1,registerLeft,1,samples);
    
    vDSP_vindex(waveLo,waveIndexArray,1,waveOct,1,samples);
    vDSP_vsmul(waveOct,1,&o,waveOct,1,samples);
    vDSP_vindex(waveLo2,waveIndexArray,1,waveOct2,1,samples);
    vDSP_vsmul(waveOct2,1,&o2,waveOct2,1,samples);
    vDSP_vadd(waveOct,1,waveOct2,1,waveOct,1,samples);
    vDSP_vmul(waveOct,1,dNotArray,1,registerRight,1,samples);
    
    //eNotArray = eNotArray * (dArray * waveMix[0][1] + dNotArray * waveMix[0][0])   --verified
    vDSP_vadd(registerLeft,1, registerRight,1, registerLeft,1, samples);
    vDSP_vmul(eScaleArray,1, registerLeft,1, eScaleArray,1, samples);    
}

static inline void renderNoiseSampleMix(float* output,float notep,float currentTimbre,float deltaTimbre,unsigned long samples)
{
    float currentTimbreNot=(1-currentTimbre);
    float deltaTimbreNot=-deltaTimbre;
    
    // unSquishedTotal[i] = 
    //  (d[i] * waveMix[0][1][j[i]] + dNot[i] * waveMix[0][0][j[i]]) * eNot[i]  +
    //  (d[i] * waveMix[1][1][j[i]] + dNot[i] * waveMix[1][0][j[i]]) * e[i] 
    //   
    renderNoiseSampleMixInternal(notep,0,0,0,1,eNotArray,samples);
    renderNoiseSampleMixInternal(notep,1,0,1,1,eArray,samples);
    vDSP_vadd(eArray,1, eNotArray,1, registerLeft,1, samples);
    
    //
    //  output += v *
    //    (plNot * unSquishedTotal + waveFundamental * pl)
    
    // registerLeft = registerLeft * unSquishedTotal
    
    vDSP_vramp(&currentTimbre,&deltaTimbre,timbreArray,1,samples);
    vDSP_vmul(registerLeft,1,timbreArray,1,registerLeft,1,samples); 
    
    vDSP_vramp(&currentTimbreNot,&deltaTimbreNot,timbreArray,1,samples);
    vDSP_vindex(_waveFundamental,waveIndexArray,1,waveMixArray,1,samples);
    vDSP_vmul(waveMixArray,1,timbreArray,1,registerRight,1,samples);  
    
    // output = v * (registerLeft + registerRight)
    vDSP_vadd(registerLeft,1, registerRight,1, registerRight,1, samples);    
    vDSP_vmul(registerRight,1, vArray,1, registerRight,1, samples);    
    vDSP_vadd(output,1, registerRight,1, output,1, samples);
    
}
*/

#define WMIX_O(x,y) ((1-oPos) * waveMix[(int)oBase][x][y][j] + oPos * waveMix[((int)oBase)+1][x][y][j])
#define WMIX_E(x)  ((1-e)*WMIX_O(0,x) + (e)*WMIX_O(1,x))
#define WMIX_G     ((1-g)*WMIX_E(0) + g*WMIX_E(1))
#define WMIX_T     (WMIX_G*t + _waveFundamental[j])

float renderNoiseInnerLoopInParallel(
                                     float* output,
                                     float notep,float notepTarget,float detune,
                                     float currentTimbre,float deltaTimbre,float phase,
                                     unsigned long samples,float invSamples,
                                     float currentVolume,float deltaVolume,
                                     float currentExpr,float deltaExpr)
{
    float freq    = powf(2,(notep-33+(1-currentExpr/2)*detune*(1-notep/127))/12) * (440/(44100.0 * 32));
    float freqEnd = powf(2,(notepTarget-33+(1-currentExpr/2)*detune*(1-notep/127))/12) * (440/(44100.0 * 32));
    float freqDiff = freqEnd - freq;
    float endPhase = phase + freq*samples + 0.5*freqDiff;
    
    float oct = notep/12;
    float octEnd = notepTarget/12;
    float octDelta = (octEnd - oct)*invSamples;
    
    /*
    renderNoiseComputeWaveIndexJ(phase,cyclesPerSample, samples);
    renderNoiseComputeV(currentVolume, deltaVolume, samples);    
    renderNoiseComputeE(currentExpr, deltaExpr, samples);    
    renderNoiseSampleMix(output,notep,currentTimbre,deltaTimbre,samples);
     */
    
    for(int i=0; i<samples; i++)
    {
        float v = currentVolume + i*deltaVolume;
        float oBase = oct + octDelta*i;
        float oPos  = oBase - (int)oBase;
        float e = currentExpr + deltaExpr*i;
        float g = v*v;
        float t = currentTimbre + i*deltaTimbre;
        float rawIndex = phase + freq*i + i*i*0.5*invSamples*invSamples*freqDiff;
        int j = ((int)(rawIndex*WAVEMAX))%WAVEMAX;
        output[i] += v * WMIX_T;
    }
    return endPhase;
}


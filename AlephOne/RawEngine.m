//
//  RawEngine.m
//  AlephOne
//
//  Created by Robert Fielding on 2/2/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//
// Only using ObjC internally to use internal classes...trying not to leak this requirement everywhere.

#import <Foundation/Foundation.h>
#import <AudioUnit/AUComponent.h>
#import <AudioUnit/AudioUnitProperties.h>
#import <AudioUnit/AudioOutputUnit.h>
#import <AudioToolbox/AudioServices.h>
#import <AVFoundation/AVFoundation.h>
#import <Accelerate/Accelerate.h>

#import "RawEngine.h"
#include "FretlessCommon.h"

AudioComponentInstance audioUnit;
AudioStreamBasicDescription audioFormat;
static const float kSampleRate = 44100.0;
static const unsigned int kOutputBus = 0;

#define ECHOBUFFERMAX (1024*2)
#define WAVEMAX (1024)
#define UNISONMAX 2
//#define FLOATRESOLUTION (1024*8)
#define HARMONICSMAX 32
#define REVERBECHOES 4

struct ramp {
    float stopValue;
    float slope;
    float value;
};

struct fingerData {
    struct ramp pitchRamp;
    struct ramp volRamp;
    struct ramp exprRamp;
    float phases[UNISONMAX];
};

#define NTSTATE_NO 0
#define NTSTATE_NEED_OFF 1
#define NTSTATE_FINISH_ON 2

struct fingersData {
    struct fingerData finger[FINGERMAX];
    float  total[UNISONMAX][WAVEMAX];
    unsigned long  sampleCount;
    int   noteTieState;
    int   otherChannel;
};




float echoBufferL[ECHOBUFFERMAX];
float echoBufferR[ECHOBUFFERMAX];
//float compressor[FLOATRESOLUTION];

#define DIST 2
#define EXPR 2
float _harmonicsTotal[EXPR][DIST];
float _harmonics[HARMONICSMAX][EXPR][DIST] =
{
    {{16,0},{0,0}}, 
    {{8,0},{0,0}}, 
    {{4,0},{0,0}}, 
    {{2,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{1,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{1,0},{1,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{1,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}, 
    {{0,0},{0,0}}    
};

float waveMix[EXPR][DIST][WAVEMAX];
float waveFundamental[WAVEMAX];


int reverbDataL[REVERBECHOES] =
{
    1131, 181, 339, 230, 1437, 485, 310, 1569, 771    
};

int reverbDataR[REVERBECHOES] =
{
    419, 586, 1450, 901, 545, 1119, 383, 231, 759  
};

float reverbStrength[REVERBECHOES] =
{
    1.0/2, 1.0/3, 1.0/4, 1.0/4, 1.0/5, 1.0/6, 1.0/7, 1.0/8, 1.0/9, 1.0/10
};

static struct fingersData allFingers;

static void moveRamps(int dstFinger, int srcFinger)
{
    if(srcFinger != dstFinger)
    {
        bcopy(&allFingers.finger[srcFinger],&allFingers.finger[dstFinger],sizeof(struct fingerData));
        allFingers.finger[srcFinger].volRamp.value = 0;
        allFingers.finger[srcFinger].volRamp.stopValue = 0;     
    }
}


static inline void setRamp(struct ramp* r, float slope, float stopValue)
{
    r->stopValue = stopValue;
    r->slope = slope;
}

static inline void doRamp(struct ramp* r,long sample)
{
    r->value = r->value * (1-r->slope) + r->slope * r->stopValue;
}



/**
   Create four waves:
 
   Their harmonics:
 
     harmonics[0][0] = A ; arbitrary harmonics
     harmonics[0][1] = B ; arbitrary harmonics
 
     harmonics[1][0] = A x Sq ; A convoluted with early odd harmonics
     harmonics[1][1] = B x Sq ; B convoluted with early odd harmonics
 
   And convoluted into waveMix, where the intent is to
   use weighted sums of the 4 possible waves.
 */
static void setExprMix()
{
    //Compute the squared off versions of our waves (not yet normalized)
    for(int expr=0; expr<EXPR; expr++)
    {
        //Convolute non distorted harmonics with square wave harmonics
        //O(n^2) with respect to the number of harmonics, but it's only called on startup
        for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
        {            
            _harmonics[harmonic][expr][1] = 0;
            for(int squareHarmonic=0; squareHarmonic<HARMONICSMAX; squareHarmonic++)
            {
                int s = squareHarmonic*2+1;
                if(s<HARMONICSMAX)
                {
                    _harmonics[s][expr][1] += _harmonics[harmonic][expr][0] / s;                                                                    
                }
            }
        }
    }
    
    
    //Normalize the harmonics
    for(int dist=0; dist<DIST; dist++)
    {
        for(int expr=0; expr<EXPR; expr++)
        {
            _harmonicsTotal[expr][dist] = 0;
            for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
            {
                _harmonicsTotal[expr][dist] += _harmonics[harmonic][expr][dist];                
            }
            for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
            {
                _harmonics[harmonic][expr][dist] /= _harmonicsTotal[expr][dist];                
            }
        }
    }  
    
    //Convolute into the wave buffers
    for(int dist=0; dist<DIST; dist++)
    {
        for(int expr=0; expr<EXPR; expr++)
        {
            for(int sample=0; sample<WAVEMAX; sample++)
            {
                waveMix[dist][expr][sample] = 0;                
                waveFundamental[sample] = sinf( sample * 2.0 * M_PI / WAVEMAX );
            }
            for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
            {
                float h = harmonic+1;
                float v = _harmonics[harmonic][expr][dist];
                for(int sample=0; sample<WAVEMAX; sample++)
                {
                    waveMix[dist][expr][sample] += sinf(h * sample * 2.0 * M_PI / WAVEMAX) * v;
                }
            }
        }
    }
}

static void audioSessionInterruptionCallback(void *inUserData, UInt32 interruptionState) {
    if (interruptionState == kAudioSessionEndInterruption) {
        AudioSessionSetActive(YES);
        AudioOutputUnitStart(audioUnit);
    }
    
    if (interruptionState == kAudioSessionBeginInterruption) {
        AudioOutputUnitStop(audioUnit);
    }
}

static void initNoise()
{
    setExprMix();
    bzero(echoBufferL,sizeof(float)*ECHOBUFFERMAX);
    bzero(echoBufferR,sizeof(float)*ECHOBUFFERMAX);
}


static inline void renderNoisePrepare(int f)
{
    if(allFingers.finger[f].volRamp.value == 0)
    {
        for(int i=0; i<UNISONMAX; i++)
        {
            allFingers.finger[f].phases[i] = 0;
        }
        allFingers.finger[f].pitchRamp.value = allFingers.finger[f].pitchRamp.stopValue;
        allFingers.finger[f].exprRamp.value = allFingers.finger[f].exprRamp.stopValue;
    }
    doRamp(&allFingers.finger[f].pitchRamp,allFingers.sampleCount);    
}

static inline void renderNoiseCleanAll(long* dataL, long* dataR,unsigned long samples)
{
    for(int phaseIdx=0; phaseIdx<UNISONMAX; phaseIdx++)
    {
        /*
        for(int i=0; i<samples; i++)
        {
            allFingers.total[phaseIdx][i] = 0;
        }*/
         
        //Huh?  Is the inlining causing the latency?  Isn't this a vector operation internally?
        bzero(allFingers.total[phaseIdx],sizeof(float)*samples);        
    }    
}

static inline void reverbConvolute(long* dataL, long* dataR,unsigned long samples)
{
    long sc = allFingers.sampleCount;
    
    for(int r=0; r<REVERBECHOES; r++)
    {
        float invR = reverbStrength[r];
        for(int i=0; i<samples; i++)
        {
            int n = (i+sc)%ECHOBUFFERMAX;
            int nL = (i+sc+reverbDataL[r])%ECHOBUFFERMAX;
            int nR = (i+sc+reverbDataR[r])%ECHOBUFFERMAX;
            echoBufferL[nL] += (0.125*echoBufferL[n] + 0.12*echoBufferR[n])*invR;
            echoBufferR[nR] += (0.125*echoBufferR[n] + 0.12*echoBufferL[n])*invR;
        }
    }
     
    for(int i=0; i<samples; i++)
    {
        int n = (i+sc)%ECHOBUFFERMAX;
        dataL[i] = INT_MAX * 0.01 * 0.25 * echoBufferL[n];
        dataR[i] = INT_MAX * 0.01 * 0.25 * echoBufferR[n];        
        echoBufferL[n] *= 0.33;
        echoBufferR[n] *= 0.33;
    }
}

static inline float compress(float f)
{
    //TODO: a fast atan approximation that doesn't call outside of here.
    // latency seems to be the issue here (not throughput)
    //return compressor[f>=1 ? (FLOATRESOLUTION-1) : ((f <= -1) ? 0 : (int)((f+1)*(FLOATRESOLUTION/2)))];
    
    return atanf(f * 2);
}

static inline void renderNoiseToBuffer(unsigned long samples,unsigned long sc)
{
    //Add pre-chorus sound together compressed
    for(int phaseIdx=0; phaseIdx<UNISONMAX; phaseIdx++)
    {
        for(int i=0; i<samples; i++)
        {
            //Is the atanf bad?
            float valL = compress(allFingers.total[phaseIdx][i]);
            float valR = valL;
            int n = (i+sc)%ECHOBUFFERMAX;
            echoBufferL[n] += valL;
            echoBufferR[n] += valR;
        }
    }    
}

//KEEP THIS DATA-PARALLEL
static inline float renderNoiseInnerLoopSampleMix(float s2,float s2Not, float e,float eNot, 
                                                  float pitchLocation,int j,
                                                  float* w00,float* w01,float* w10, float* w11)
{
    float unSquished = (w10[j]*s2Not + w00[j]*(s2))*eNot + (w11[j]*s2Not + w01[j]*(s2))*e;
    return unSquished*(1-pitchLocation) + waveFundamental[j]*(pitchLocation);    
}

//KEEP THIS DATA-PARALLEL ... no branches, and no dependencies on any other values of i, or dependencies on changed values
static inline void renderNoiseInnerLoopSample(
                                              int f,int phaseIdx,int i,float cyclesPerSample,float pitchLocation,float p,float invSamples,
                                              float currentVolume,float diffVolume, float currentExpr, float diffExpr,
                                              float* w00,float* w01,float* w10, float* w11)
{
    float e = currentExpr + i * invSamples * (diffExpr);
    float v = currentVolume + i * invSamples * (diffVolume);
    float s2 = e*e;
    float eNot = (1-e);
    float s2Not = (1-s2);
    float cycles = i*cyclesPerSample + p;
    float cycleLocation = (cycles - (int)cycles); 
    int j = (int)(cycleLocation*WAVEMAX);
    float unAliased = renderNoiseInnerLoopSampleMix(s2,s2Not, e,eNot, 
                                                    pitchLocation,j,
                                                    w00,w01,w10,w11);
    allFingers.total[phaseIdx][i] += v * unAliased;
}

static void renderNoiseInnerLoop(int f,int phaseIdx,float detune,unsigned long samples,float invSamples,
                                        float currentVolume,float diffVolume, float currentExpr, float diffExpr)
{
    float notep = allFingers.finger[f].pitchRamp.value;
    float pitchLocation = notep/127.0;
    float p = allFingers.finger[f].phases[phaseIdx];
    //note 33 is our center pitch, and it's 440hz
    //powf exits out of here, but it's not per sample... 
    float cyclesPerSample = powf(2,(notep-33+(1-currentExpr)*detune*(1-pitchLocation))/12) * (440/(44100.0 * 32));
    float* w00 = waveMix[0][0];
    float* w01 = waveMix[0][1];
    float* w10 = waveMix[1][0];
    float* w11 = waveMix[1][1];
    for(int i=0; i<samples; i++)
    {
        renderNoiseInnerLoopSample(f,phaseIdx,i,cyclesPerSample,pitchLocation,p,invSamples,
                                   currentVolume,diffVolume,currentExpr,diffExpr,
                                   w00,w01,w10,w11);
    }         
    allFingers.finger[f].phases[phaseIdx] = (cyclesPerSample*samples) + p;
}

//TRY TO NOT CALL OUT TO OUTSIDE WORLD FROM HERE
static void renderNoise(long* dataL, long* dataR, unsigned long samples)
{
    renderNoiseCleanAll(dataL,dataR,samples);
    int activeFingers=0;
    float invSamples = 1.0/samples;
    //Go in channel major order because we skip by volume
    for(int f=0; f<FINGERMAX; f++)
    {
        float currentVolume = allFingers.finger[f].volRamp.value;
        float targetVolume = allFingers.finger[f].volRamp.stopValue;
        float diffVolume = (targetVolume - currentVolume);
        float currentExpr = allFingers.finger[f].exprRamp.value;
        float targetExpr = allFingers.finger[f].exprRamp.stopValue;
        float diffExpr = (targetExpr - currentExpr);
        int isActive = (currentVolume > 0) || (targetVolume > 0);
        
        if(isActive)
        {
            activeFingers++;
            renderNoisePrepare(f);
            renderNoiseInnerLoop(f,0,    0,samples, invSamples, currentVolume,diffVolume,currentExpr,diffExpr);
            if(UNISONMAX>1)
            {
                renderNoiseInnerLoop(f,1, -0.2,samples, invSamples, currentVolume,diffVolume,currentExpr,diffExpr);
                if(UNISONMAX>2)
                {
                    renderNoiseInnerLoop(f,2,  0.2,samples, invSamples, currentVolume,diffVolume,currentExpr,diffExpr);                
                }                
            }
            allFingers.finger[f].volRamp.value = targetVolume;
            allFingers.finger[f].exprRamp.value = targetExpr;
        }
    }
    renderNoiseToBuffer(samples,allFingers.sampleCount);
    reverbConvolute(dataL,dataR,samples);
    allFingers.sampleCount += samples;
}

void rawEngine(int midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr)
{
    //printf("%d %d %f %f %d %d\n",midiChannel, doNoteAttack, pitch, volVal, midiExprParm, midiExpr);
    int channel = midiChannel;
    //Handle the note-tie state machine.  We expect the note-off to come before note-on in the note-tie,
    //which could be the opposite decision of other implementers.
    int doVol = 1;
    if(doNoteAttack)
    {
        //We got signalled to expect a note tie next time!
        allFingers.noteTieState = NTSTATE_NEED_OFF;
    }
    else
    {
        if(allFingers.noteTieState == NTSTATE_NEED_OFF)
        {
            if(volVal == 0)
            {
                allFingers.noteTieState = NTSTATE_FINISH_ON;
                allFingers.otherChannel = channel;
                doVol = 0;
            }
        }
        else
        {
            if(allFingers.noteTieState == NTSTATE_FINISH_ON)
            {
                if(volVal > 0)
                {
                    allFingers.noteTieState = NTSTATE_NO;
                    moveRamps(channel,allFingers.otherChannel);
                }
            }
        }
        if(doVol) //If we are in legato, then this might need to be stopped
        {
            setRamp(&allFingers.finger[channel].volRamp, 0.008 * pitch/127.0 * ((volVal==0)?0.25:1), volVal);            
        }
        if(volVal!=0) //Don't bother with ramping these on release
        {
            setRamp(&allFingers.finger[channel].pitchRamp, 0.9, pitch);
            setRamp(&allFingers.finger[channel].exprRamp, 0.1, midiExpr/127.0);                                            
        }
    }
}

static OSStatus fixGDLatency()
{
    float latency = 0.005;
    
    OSStatus
    status = AudioSessionSetProperty(
                                     kAudioSessionProperty_PreferredHardwareIOBufferDuration,
                                     sizeof(latency),&latency
                                     );
    if(status != 0)
    {
    }
    UInt32 allowMixing = true;
    status = AudioSessionSetProperty(
                                     kAudioSessionProperty_OverrideCategoryMixWithOthers,
                                     sizeof(allowMixing),&allowMixing
                                     );
    
    AudioStreamBasicDescription auFmt;
    UInt32 auFmtSize = sizeof(AudioStreamBasicDescription);
    
    AudioUnitGetProperty(audioUnit,
                         kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Output,
                         kOutputBus,
                         &auFmt,
                         &auFmtSize);
    return status;
}



void SoundEngine_wake()
{
    NSError *categoryError = nil;
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayback error:&categoryError];
    fixGDLatency();
    initNoise();
}


static OSStatus SoundEngine_playCallback(void *inRefCon,
                                         AudioUnitRenderActionFlags *ioActionFlags,
                                         const AudioTimeStamp *inTimeStamp,
                                         UInt32 inBusNumber,
                                         UInt32 inNumberFrames,
                                         AudioBufferList *ioData)
{
    assert(inBusNumber == kOutputBus);
    
    AudioBuffer* outputBufferL = &ioData->mBuffers[0];
    SInt32* dataL = (SInt32*)outputBufferL->mData;
    AudioBuffer* outputBufferR = &ioData->mBuffers[1];
    SInt32* dataR = (SInt32*)outputBufferR->mData;
    
    renderNoise(dataL,dataR,inNumberFrames);
    return 0;
}

void SoundEngine_start()
{
    // Initialize the audio session.
    AudioSessionInitialize(NULL, NULL, audioSessionInterruptionCallback, NULL);
        
    OSStatus status;
    // Describe audio component
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    // Get component
    AudioComponent outputComponent = AudioComponentFindNext(NULL, &desc);
    
    // Get audio units
    status = AudioComponentInstanceNew(outputComponent, &audioUnit);
    if(status)
    {
        NSLog(@"AudioComponentInstanceNew failed: %ld",status);
    }
    else
    {
        // Enable playback
        UInt32 enableIO = 1;
        status = AudioUnitSetProperty(audioUnit,
                                      kAudioOutputUnitProperty_EnableIO,
                                      kAudioUnitScope_Output,
                                      kOutputBus,
                                      &enableIO,
                                      sizeof(UInt32));
        if(status)
        {
            NSLog(@"AudioUnitSetProperty enable io failed: %ld",status);
        }
        else
        {
            audioFormat.mSampleRate = 44100.0;
            audioFormat.mFormatID = kAudioFormatLinearPCM;
            audioFormat.mFormatFlags = kAudioFormatFlagsAudioUnitCanonical;
            audioFormat.mBytesPerPacket = sizeof(AudioUnitSampleType);
            audioFormat.mFramesPerPacket = 1;
            audioFormat.mBytesPerFrame = sizeof(AudioUnitSampleType);
            audioFormat.mChannelsPerFrame = 2;
            audioFormat.mBitsPerChannel = 8 * sizeof(AudioUnitSampleType);
            audioFormat.mReserved = 0;
            // Apply format
            status = AudioUnitSetProperty(audioUnit,
                                          kAudioUnitProperty_StreamFormat,
                                          kAudioUnitScope_Input,
                                          kOutputBus,
                                          &audioFormat,
                                          sizeof(AudioStreamBasicDescription));
            if(status)
            {
                NSLog(@"AudioUnitSetProperty stream format failed: %ld",status);
            }
            else
            {
                UInt32 maxFrames = 1024*4;
                AudioUnitSetProperty(audioUnit,
                                     kAudioUnitProperty_MaximumFramesPerSlice,
                                     kAudioUnitScope_Input,
                                     kOutputBus,
                                     &maxFrames,
                                     sizeof(maxFrames));
                
                AURenderCallbackStruct callback;
                callback.inputProc = &SoundEngine_playCallback;
                
                // This is only required if you want the pointer to the object to be passed into the function.
                //callback.inputProcRefCon = self; ???? Is that required
                
                // Set output callback
                status = AudioUnitSetProperty(audioUnit,
                                              kAudioUnitProperty_SetRenderCallback,
                                              kAudioUnitScope_Global,
                                              kOutputBus,
                                              &callback,
                                              sizeof(AURenderCallbackStruct));
                if(status)
                {
                    NSLog(@"AudioUnitSetProperty render callback failed: %ld",status);
                }
                else
                {
                    status = AudioUnitInitialize(audioUnit);
                    if(status)
                    {
                        NSLog(@"AudioUnitSetProperty initialize failed: %ld",status);
                    }
                    else
                    {
                        status = AudioOutputUnitStart(audioUnit);
                        if(status)
                        {
                          NSLog(@"AudioUnitStart:%ld",status);
                        }
                        else
                        {
                            SoundEngine_wake();
                        }
                    }
                }
            }
        }
    }
}

void rawEngineStart()
{
    SoundEngine_start();
    NSLog(@"rawEngineStart");
}

void rawEngineStop()
{
    AudioOutputUnitStop(audioUnit);    
    NSLog(@"rawEngineStop");    
}



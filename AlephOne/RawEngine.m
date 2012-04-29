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
#import "RawEngineGenerated.h"
#include "FretlessCommon.h"
#include "Parameters.h"

#define ECHOBUFFERMAX (1024*32)
#define UNISONMAX 3
#define HARMONICSMAX 32
#define REVERBECHOES 10
#define AUDIOCHANNELS 2

AudioComponentInstance audioUnit;
AudioStreamBasicDescription audioFormat;
static const float kSampleRate = 44100.0;
static const unsigned int kOutputBus = 0;
static unsigned long lastSamples = 256;

static void audioSessionInterruptionCallback(void *inUserData, UInt32 interruptionState) {
    if (interruptionState == kAudioSessionEndInterruption) {
        AudioSessionSetActive(YES);
        AudioOutputUnitStart(audioUnit);
    }
    
    if (interruptionState == kAudioSessionBeginInterruption) {
        AudioOutputUnitStop(audioUnit);
    }
}



struct ramp {
    float stopValue;
    float finalValue;
    int   buffers;
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
    float  total[UNISONMAX][WAVEMAX] __attribute__ ((aligned));
    unsigned long  sampleCount;
    int   noteTieState;
    int   otherChannel;
    struct fingerData finger[FINGERMAX];

    struct ramp reverbRamp;
    struct ramp detuneRamp;
    struct ramp timbreRamp;
    struct ramp distRamp;
};


float echoBufferL[ECHOBUFFERMAX] __attribute__ ((aligned));
float echoBufferR[ECHOBUFFERMAX] __attribute__ ((aligned));
//float convBufferL[ECHOBUFFERMAX] __attribute__ ((aligned));
//float convBufferR[ECHOBUFFERMAX] __attribute__ ((aligned));

float unisonDetune[UNISONMAX] = {
    0, -0.25, 0.25    
};
float unisonVol[UNISONMAX] = {
  1, 0.75, 0.75  
};

////This is how we define harmonic content... a single cycle wave for each point along the 2D axis
float _harmonicsTotal         [EXPR][DIST];
float _harmonics[HARMONICSMAX][EXPR][DIST] =
{
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

int reverbDataL[REVERBECHOES] __attribute__ ((aligned)) =
{
  3,73,339,230*6+1,1437*9+1,893*8,310*7+1,1569*7+1,771*8+1  
};
int reverbDataR[REVERBECHOES] __attribute__ ((aligned)) =
{
  5,97,1450,901*6+1,545*4,533*8+1,383*10+1,231*5+1,759*6+1,234*7+1  
};

float reverbStrength[REVERBECHOES] __attribute__ ((aligned)) =
{
    0.5, 0.5, 0.64, 0.6, 0.65, 0.53, 0.4, 0.4, 0.7, 0.9
};

static struct fingersData allFingers;

static void moveRamps(int dstFinger, int srcFinger)
{
    if(srcFinger != dstFinger)
    {
        bcopy(&allFingers.finger[srcFinger],&allFingers.finger[dstFinger],sizeof(struct fingerData));
        //No impulsing happens because it's on a different finger now
        allFingers.finger[srcFinger].volRamp.value = 0;
        allFingers.finger[srcFinger].volRamp.stopValue = 0;    
        allFingers.finger[srcFinger].volRamp.finalValue = 0;
    }
}


static inline void setRamp(struct ramp* r, int buffers, float finalValue)
{
    r->finalValue = finalValue;
    r->buffers = buffers;
    r->stopValue = ((r->buffers-1) * r->value + r->finalValue) / (r->buffers);
}


//Do this ramp once per buffer
static inline void doRamp(struct ramp* r)
{
    r->value = r->stopValue;
    if(r->buffers>1)
    {
        setRamp(r,r->buffers-1,r->finalValue);
    }
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
static void initNoise()
{
    bzero(echoBufferL,sizeof(float)*ECHOBUFFERMAX);
    bzero(echoBufferR,sizeof(float)*ECHOBUFFERMAX);
    
    for(int i=0; i<SAMPLESMAX; i++)
    {
        sampleIndexArray[i] = i;
    }
    
    for(int i=0; i<16; i++)
    {
        _harmonics[i][0][0] = 16-i;
    }
    _harmonics[0][1][0] = 1;
    _harmonics[1][1][0] = 2;
    _harmonics[3][1][0] = 4;
    _harmonics[7][1][0] = 8;
    
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
                waveMix[expr][dist][sample] = 0;                
                _waveFundamental[sample] = sinf( sample * 2.0 * M_PI / WAVEMAX );
            }
            for(int harmonic=0; harmonic<HARMONICSMAX/(1+expr); harmonic++)
            {
                float h = harmonic+1;
                float v = _harmonics[harmonic][expr][dist];
                for(int sample=0; sample<WAVEMAX; sample++)
                {
                    waveMix[expr][dist][sample] += sinf(h * sample * 2.0 * M_PI / WAVEMAX) * v;
                }
            }
        }
    }
}

static inline void renderNoisePrepare(int f)
{
    if(allFingers.finger[f].volRamp.value == 0)
    {
        //Don't do ramping if we are currently silent
        for(int i=0; i<UNISONMAX; i++)
        {
            allFingers.finger[f].phases[i] = 0;
        }
        
        allFingers.finger[f].pitchRamp.value = allFingers.finger[f].pitchRamp.finalValue;
        allFingers.finger[f].exprRamp.value = allFingers.finger[f].exprRamp.finalValue;
    }
}

static inline void renderNoiseCleanAll(unsigned long samples)
{
    for(int u=0; u<UNISONMAX; u++)
    {
        vDSP_vclr(allFingers.total[u],1,samples);        
    }
}


static inline void xDSP_vcp(float* src,float* dst,int count)
{
    memcpy(dst,src,count*sizeof(float));
}

static inline float compress(float f)
{
    return atanf(f);
}

static inline void renderNoiseToBuffer(long* dataL, long* dataR, unsigned long samples)
{
    //Distortion goes to 11
    float innerScale = 0.5+10.5*(allFingers.distRamp.value);
    
    //Scale to fit range when converting to integer
    float scaleFactor = 0x800000 * 2.0/M_PI;
    
    long sc = allFingers.sampleCount;
    float reverbAmount = (allFingers.reverbRamp.value);
    
    float invr[REVERBECHOES];
    
    for(int r=0; r<REVERBECHOES; r++)
    {
        invr[r] = (reverbStrength[r]) * reverbAmount;        
    }
    
    //Add pre-chorus sound together compressed
    for(int i=0; i<samples; i++)
    {
        int n = (i+sc)%ECHOBUFFERMAX;
        for(int phaseIdx=0; phaseIdx<UNISONMAX; phaseIdx++)
        {
            //Is the atanf bad?
            float val = compress(allFingers.total[phaseIdx][i] * innerScale);
            echoBufferL[n] += val;
            echoBufferR[n] += val;
        }
        
        echoBufferL[n] = atanf(echoBufferL[n] * 0.25);
        echoBufferR[n] = atanf(echoBufferR[n] * 0.25);
        
        int sci = i+sc;
        for(int r=0; r<REVERBECHOES; r++)
        {
            int nL = sci+reverbDataL[r];
            int nR = sci+reverbDataR[r];
            float vL = echoBufferL[n]*invr[r];
            float vR = echoBufferR[n]*invr[r];
            float s = 1;
            for(int n=0;n<4;n++)
            {
                s *= 0.5;
                nL+=reverbDataL[r];
                nR+=reverbDataR[r];
                int nLX = nL % ECHOBUFFERMAX;
                int nRX = nR % ECHOBUFFERMAX;
                echoBufferL[nLX] += s*vR;
                echoBufferR[nRX] += s*vL;
            }
        }
        dataL[i] = scaleFactor * atanf(echoBufferL[n] * 0.75);
        dataR[i] = scaleFactor * atanf(echoBufferR[n] * 0.75);        
        echoBufferL[n] = 0;
        echoBufferR[n] = 0;
    }    
}


static void renderNoiseInnerLoop(int f,unsigned long samples,float invSamples,
                                        float currentVolume,float diffVolume, float currentExpr, float diffExpr)
{
    float notep = allFingers.finger[f].pitchRamp.value;
    float pitchLocation = notep/127.0;
    //note 33 is our center pitch, and it's 440hz
    //powf exits out of here, but it's not per sample... 
    for(int u=0; u<UNISONMAX; u++)
    {
        float uVol = unisonVol[u];
        if(u>0)
        {
            uVol *= allFingers.detuneRamp.value;
        }
        float phase = allFingers.finger[f].phases[u];
        allFingers.finger[f].phases[u] = 
        renderNoiseInnerLoopInParallel(
                                           allFingers.total[u],notep,unisonDetune[u]*allFingers.detuneRamp.value,
                                           pitchLocation,phase,
                                           samples,invSamples,
                                           currentVolume*uVol,diffVolume*invSamples*uVol,
                                           currentExpr,diffExpr*invSamples, allFingers.timbreRamp.value);
    }
}

static void renderNoise(long* dataL, long* dataR, unsigned long samples)
{
    renderNoiseCleanAll(samples);
    int activeFingers=0;
    float invSamples = 1.0/samples;
    
    setRamp(&allFingers.reverbRamp, 8, getReverb());
    setRamp(&allFingers.detuneRamp, 8, getDetune());
    setRamp(&allFingers.timbreRamp, 8, getTimbre());
    setRamp(&allFingers.distRamp, 8, getDistortion());
    
    //Go in channel major order because we skip by volume
    for(int f=0; f<FINGERMAX; f++)
    {
        float currentVolume = allFingers.finger[f].volRamp.value;
        float targetVolume  = allFingers.finger[f].volRamp.stopValue;
        float finalVolume   = allFingers.finger[f].volRamp.finalValue;
        int isActive        = (currentVolume > 0) || (finalVolume > 0) || (targetVolume > 0);
        
        if(isActive)
        {
            float diffVolume    = (targetVolume - currentVolume);
            float currentExpr   = allFingers.finger[f].exprRamp.value;
            float targetExpr    = allFingers.finger[f].exprRamp.stopValue;
            float diffExpr      = (targetExpr - currentExpr);
            
            activeFingers++;
            renderNoisePrepare(f);
            renderNoiseInnerLoop(f,samples, invSamples, currentVolume,diffVolume,currentExpr,diffExpr);                
            doRamp(&allFingers.finger[f].pitchRamp);    
            doRamp(&allFingers.finger[f].exprRamp);                
            doRamp(&allFingers.finger[f].volRamp);    
        }
    }
    renderNoiseToBuffer(dataL,dataR,samples);
    doRamp(&allFingers.reverbRamp);
    doRamp(&allFingers.detuneRamp);
    doRamp(&allFingers.timbreRamp);
    doRamp(&allFingers.distRamp);
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
            //int goingDown = (pitch==0)?4:1;
            //float rampVal = (1 + 32 * (128-pitch)/127.0)*goingDown;
            setRamp(&allFingers.finger[channel].volRamp, 1, volVal);            
        }
        if(volVal!=0) //Don't bother with ramping these on release
        {
            setRamp(&allFingers.finger[channel].pitchRamp, 1, pitch);
            setRamp(&allFingers.finger[channel].exprRamp, 1, midiExpr/127.0);                                            
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
    lastSamples = inNumberFrames;
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
            audioFormat.mChannelsPerFrame = AUDIOCHANNELS;
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



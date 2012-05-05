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

#define LOOPBUFFERMAX (1024*1024)
#define ECHOBUFFERMAX (1024*128)
#define UNISONMAX 3
#define HARMONICSMAX 32
#define REVERBECHOES 10
#define AUDIOCHANNELS 2

BOOL audioIsRunning = FALSE;
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
    struct ramp pitchLocation;
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

float loopBufferL[LOOPBUFFERMAX] __attribute__ ((aligned));
float loopBufferR[LOOPBUFFERMAX] __attribute__ ((aligned));
int loopIndexStart = 0;
int loopIndexRepeat = 0;
int loopRecording = 0;
int loopPlaying = 0;

float echoBufferL[ECHOBUFFERMAX] __attribute__ ((aligned));
float echoBufferR[ECHOBUFFERMAX] __attribute__ ((aligned));
//float convBufferL[ECHOBUFFERMAX] __attribute__ ((aligned));
//float convBufferR[ECHOBUFFERMAX] __attribute__ ((aligned));

float octaveHarmonicLimit[OCTAVES] = {
  32,32,32,32,32,24,16,8,4,2,2,1    
};

float unisonDetune[UNISONMAX] = {
    0, -0.25, 0.25    
};
float unisonVol[UNISONMAX] = {
  1, 1, 1  
};

float harmonics[EXPR][DIST][HARMONICSMAX];

int reverbDataL[REVERBECHOES] __attribute__ ((aligned)) =
{
  //17,73*4+1,339*3+1,230*3+1,1437*3+1,893*8,310*1+1,1569*7+1 
  3,7,13,19,29,37,43,49,53,59
};
int reverbDataR[REVERBECHOES] __attribute__ ((aligned)) =
{
  //41,97*3+1,1450*2+1,901*4+1,545*4,533*2+1,383*8+1,231*3+1 
  5,11,17,23,31,41,47,51,57,61
};

float reverbStrength[REVERBECHOES] __attribute__ ((aligned)) =
{
    0.99, 0.99, 0.98, 0.99, 0.99, 0.99, 0.99, 0.99,0.9,0.9
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
 

static void clearHarmonics(float* harmonicsArr)
{
    bzero(harmonicsArr,sizeof(float)*HARMONICSMAX);
}

static void normalizeHarmonics(float* harmonicsArr)
{
    float totalHarmonics = 0;
    for(int h=0; h<HARMONICSMAX; h++)
    {
        totalHarmonics += harmonicsArr[h];                
    }
    if(totalHarmonics != 0)
    {
        for(int h=0; h<HARMONICSMAX; h++)
        {
            harmonicsArr[h] /= totalHarmonics;
        }        
    }
}

static void setupSampleIndexArray()
{
    for(int i=0; i<SAMPLESMAX; i++)
    {
        sampleIndexArray[i] = i;
    }    
}

static void initNoise()
{
    //Tune to D
    int noteInSamples = (3*50)/4;
    for(int i=0;i<4;i++)
    {
        reverbDataL[i] *= noteInSamples*2;
        reverbDataR[i] *= noteInSamples*2;
    }
    for(int i=4;i<8;i++)
    {
        reverbDataL[i] *= noteInSamples*5;
        reverbDataR[i] *= noteInSamples*5;
    }
    
    bzero(loopBufferL,sizeof(float)*LOOPBUFFERMAX);
    bzero(loopBufferR,sizeof(float)*LOOPBUFFERMAX);
    
    bzero(echoBufferL,sizeof(float)*ECHOBUFFERMAX);
    bzero(echoBufferR,sizeof(float)*ECHOBUFFERMAX);
    setupSampleIndexArray();
    
    clearHarmonics(harmonics[0][0]);
    for(int i=0; i<HARMONICSMAX; i++)
    {
        harmonics[0][0][i] = 1.0/(i+1);
    }
    normalizeHarmonics(harmonics[0][0]);

    clearHarmonics(harmonics[1][0]);
    for(int i=0; i<HARMONICSMAX/2; i++)
    {
        harmonics[1][0][2*i+1] = 1.0/(i+1);
    }
    normalizeHarmonics(harmonics[1][0]);
    //Make squared versions of our waves
    for(int expr=0; expr<EXPR; expr++)
    {
        clearHarmonics(harmonics[expr][1]);
        for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
        {            
            for(int squareHarmonic=0; squareHarmonic<HARMONICSMAX; squareHarmonic++)
            {
                int s = squareHarmonic*2+1;
                if(s<HARMONICSMAX)
                {
                    harmonics[expr][1][s-1] += harmonics[expr][0][harmonic] / s;                                                                    
                }
            }
        }
        normalizeHarmonics(harmonics[expr][1]);
    }
    
    //Convolute into the wave buffers
    for(int oct=0; oct<OCTAVES; oct++)
    {
        for(int dist=0; dist<DIST; dist++)
        {
            for(int expr=0; expr<EXPR; expr++)
            {
                for(int sample=0; sample<WAVEMAX; sample++)
                {
                    waveMix[oct][expr][dist][sample] = 0;                
                    _waveFundamental[sample] = sinf( 1 * sample * 2.0 * M_PI / WAVEMAX );
                }
                for(int harmonic=0; harmonic<HARMONICSMAX && harmonic<octaveHarmonicLimit[oct]; harmonic++)
                {
                    float h = harmonic+1;
                    float harmonicWeight = harmonics[expr][dist][harmonic];
                    for(int sample=0; sample<WAVEMAX; sample++)
                    {
                        waveMix[oct][expr][dist][sample] += sinf(h * sample * 2.0 * M_PI / WAVEMAX) * harmonicWeight;
                    }
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
        allFingers.finger[f].pitchLocation.value = allFingers.finger[f].pitchLocation.finalValue;
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
    float dist = (allFingers.distRamp.value);
    float noDist = 1 - dist;
    dist=dist*dist;
    float innerScale = (0.5+10.5*dist)*0.5;
    
    //Scale to fit range when converting to integer
    float scaleFactor = 0x800000 * 2.0/M_PI;
    
    long sc = allFingers.sampleCount;
    float reverbAmount = (allFingers.reverbRamp.value * allFingers.reverbRamp.value);
    float noReverbAmount = (1 - reverbAmount);
    reverbAmount = reverbAmount*0.9;
    
    //Add pre-chorus sound together compressed
    for(int i=0; i<samples; i++)
    {
        int n = (i+sc)%ECHOBUFFERMAX;
        int n2 = (i+1+sc)%ECHOBUFFERMAX;
        float rawTotal=0;
        float totalL=0;
        float totalR=0;
        float feedL = echoBufferL[n];
        float feedR = echoBufferR[n];
        float lL = loopBufferL[n] * loopPlaying;
        float lR = loopBufferR[n] * loopPlaying;
        for(int phaseIdx=0; phaseIdx<UNISONMAX; phaseIdx++)
        {
            float raw = allFingers.total[phaseIdx][i];
            //Is the atanf bad?
            float val = dist*compress(raw * innerScale) + 3*noDist*raw;
            rawTotal += val;
        }        
        //These variables determine whether we get feedback, or creeping silence.
        float totalScale = 0.3;
        float feedScale = 0.099;
        float channelBleed = 0.125;
        float finalScale = 2;
        float scaledTotal = rawTotal*totalScale;
        float feedRawL = feedL*feedScale*reverbAmount;
        float feedRawR = feedR*feedScale*reverbAmount;
        totalL = feedRawL + scaledTotal;
        totalR = feedRawR + scaledTotal;
        echoBufferL[n2] += echoBufferL[n];
        echoBufferR[n2] += echoBufferR[n];
        echoBufferL[n2] *= 0.4555;
        echoBufferR[n2] *= 0.4555;
        echoBufferL[n] = 0;
        echoBufferR[n] = 0;
        
        int sci = i+sc;
        for(int r=0; r<REVERBECHOES; r++)
        {
            int nL = sci+reverbDataL[r];
            int nR = sci+reverbDataR[r];
            float vL = totalL*reverbStrength[r];
            float vR = totalR*reverbStrength[r];
            int nLX = nL % ECHOBUFFERMAX;
            int nRX = nR % ECHOBUFFERMAX;
            echoBufferL[nLX] += vL + channelBleed*vR;
            echoBufferR[nRX] += vR + channelBleed*vL;
        }
        
        float aL = atanf(finalScale * (feedRawL + scaledTotal*noReverbAmount + lL));
        float aR = atanf(finalScale * (feedRawR + scaledTotal*noReverbAmount + lR));
        if(loopRecording)
        {
            int lN = (allFingers.sampleCount + i - loopIndexStart);
            if(loopPlaying)
            {
                loopBufferL[lN] = (loopBufferL[lN] + aL)/2;            
                loopBufferR[lN] = (loopBufferR[lN] + aR)/2;                            
            }
            else 
            {
                loopBufferL[lN] = aL;
                loopBufferR[lN] = aR;
            }
        }
        dataL[i] = scaleFactor * aL;
        dataR[i] = scaleFactor * aR;        
    }    
}


static void renderNoiseInnerLoop(int f,unsigned long samples,float invSamples,
                                        float currentVolume,float diffVolume, float currentExpr, float diffExpr,float currentPitchLocation, float deltaPitchLocation)
{
    float notep = allFingers.finger[f].pitchRamp.value;
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
                                           currentPitchLocation,deltaPitchLocation,phase,
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
    
    setRamp(&allFingers.reverbRamp, 64, getReverb());
    setRamp(&allFingers.detuneRamp, 16, getDetune());
    setRamp(&allFingers.timbreRamp, 16, getTimbre());
    setRamp(&allFingers.distRamp, 16, getDistortion());
    
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
            float currentPitchLocation = allFingers.finger[f].pitchLocation.value;
            float targetPitchLocation = allFingers.finger[f].pitchLocation.stopValue;
            float deltaPitchLocation = (targetPitchLocation - currentPitchLocation);
            
            activeFingers++;
            renderNoisePrepare(f);
            renderNoiseInnerLoop(f,samples, invSamples, currentVolume,diffVolume,currentExpr,diffExpr,currentPitchLocation,deltaPitchLocation);                
            doRamp(&allFingers.finger[f].pitchRamp);    
            doRamp(&allFingers.finger[f].pitchLocation);    
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

float findFilterLevel(float pitchLocation, float timbre)
{
    float cutoffScale = 1-timbre;
    float pitchFilter = pitchLocation*1.25;
    if(pitchFilter>1)
    {
        pitchFilter=1;
    }
    return cutoffScale * pitchFilter;
}

void loopStart()
{
    loopIndexStart = allFingers.sampleCount;
    loopRecording = 1;
}

void loopRepeat()
{
    loopIndexRepeat = allFingers.sampleCount;
    loopRecording = 0;
    loopPlaying = 1;
}

void loopPlay()
{
    loopPlaying = !loopPlaying;
}

void loopClear()
{
    loopIndexRepeat = 0;
    loopRecording = 0;
    loopPlaying = 0;
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
            int rampVal = 1;
            if(volVal == 0)
            {
                rampVal++;
            }
            setRamp(&allFingers.finger[channel].volRamp, rampVal, volVal);            
        }
        if(volVal!=0) //Don't bother with ramping these on release
        {
            setRamp(&allFingers.finger[channel].pitchRamp, 1, pitch);
            setRamp(&allFingers.finger[channel].pitchLocation, 1, findFilterLevel(pitch/127.0,getTimbre()));
            setRamp(&allFingers.finger[channel].exprRamp, 4, midiExpr/127.0);                                            
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
    audioIsRunning = TRUE;
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
    if(audioIsRunning == FALSE)
    {
        SoundEngine_start();
    }
    else 
    {
        AudioOutputUnitStart(audioUnit);
    }
    NSLog(@"rawEngineStart");
}

void rawEngineStop()
{
    AudioOutputUnitStop(audioUnit);    
    NSLog(@"rawEngineStop");    
}



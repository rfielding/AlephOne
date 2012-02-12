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


#import "RawEngine.h"

AudioComponentInstance audioUnit;
AudioStreamBasicDescription audioFormat;
static const float kSampleRate = 44100.0;
static const unsigned int kOutputBus = 0;

#define WAVEMAX (2048)
#define MAXCHANNELS 16
#define EXPRLEVELS 64

struct ramp {
    long startSample;
    long stopSample;
    float startValue;
    float stopValue;
    float value;
};

struct fingerData {
    struct ramp pitchRamp;
    struct ramp volRamp;
    struct ramp exprRamp;
    float phase;
};

struct fingersData {
    struct fingerData finger[MAXCHANNELS];
    long  sampleCount;
};

static struct fingersData allFingers;

void setRamp(struct ramp* r, long samples, float stopValue)
{
    r->startValue = r->stopValue;
    r->startSample = allFingers.sampleCount;
    r->stopValue = stopValue;
    r->stopSample = r->startSample + samples;
}

void doRamp(struct ramp* r)
{
    long sampleDiff = r->stopSample - r->startSample;
    r->value = r->startValue + r->stopValue * sampleDiff;
}

void doRamps()
{
    for(int f=0; f<MAXCHANNELS; f++)
    {
        doRamp(&allFingers.finger[f].pitchRamp);
        doRamp(&allFingers.finger[f].volRamp);
        doRamp(&allFingers.finger[f].exprRamp);
    }
}

float notePitch[MAXCHANNELS];
float noteVol[MAXCHANNELS];
float notePitchTarget[MAXCHANNELS];
float noteVolTarget[MAXCHANNELS];
float notePhase[MAXCHANNELS];
float noteExpr[MAXCHANNELS];
float noteExprTarget[MAXCHANNELS];
float totalNoteVolume=0;



#define HARMONICSMAX 32
float waveMix[2][2][WAVEMAX];
float harmonicsTotal[2][2];
float harmonics[2][2][HARMONICSMAX] =
{
    {
        {8, 4, 1, 2, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    },
    {
        {8, 4, 1, 2, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  
        {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    },
};


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
static void setupWaves()
{

    
    //Compute the squared off versions of our waves (not yet normalized)
    for(int expr=0; expr<2; expr++)
    {
        for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
        {
            harmonics[1][expr][harmonic] = 0;                        
        }
        //Convolute non distorted harmonics with square wave harmonics
        for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
        {            
            for(int squareHarmonic=0; squareHarmonic<5; squareHarmonic++)
            {
                int s = squareHarmonic*2+1;
                if(s<HARMONICSMAX)
                {
                    harmonics[1][expr][s] += harmonics[0][expr][harmonic] / s;                                                                    
                }
            }
            /*
            for(int squareHarmonic=0; squareHarmonic<10; squareHarmonic++)
            {
                int s = squareHarmonic+1;
                if(s<HARMONICSMAX)
                {
                    harmonics[1][expr][s] += harmonics[0][expr][harmonic] / s;                                                                    
                }
            }
             */
        }
    }
    
    
    //Normalize the harmonics
    for(int dist=0; dist<2; dist++)
    {
        for(int expr=0; expr<2; expr++)
        {
            harmonicsTotal[dist][expr] = 0;
            for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
            {
                harmonicsTotal[dist][expr] += harmonics[dist][expr][harmonic];                
            }
            for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
            {
                harmonics[dist][expr][harmonic] /= harmonicsTotal[dist][expr];                
            }
        }
    }  
    

    
    //Convolute into the wave buffers
    for(int dist=0; dist<2; dist++)
    {
        for(int expr=0; expr<2; expr++)
        {
            for(int sample=0; sample<WAVEMAX; sample++)
            {
                waveMix[dist][expr][sample] = 0;                
            }
            for(int harmonic=0; harmonic<HARMONICSMAX; harmonic++)
            {
                float h = harmonic+1;
                float v = harmonics[dist][expr][harmonic];
                for(int sample=0; sample<WAVEMAX; sample++)
                {
                    waveMix[dist][expr][sample] += sinf(h * sample * 2.0 * M_PI / WAVEMAX) * v;
                }
            }
        }
    }
}

/*
   We set two sets of harmonic content, and expr fades between the two of them.
 */


static void setExprMix()
{
    setupWaves();
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
    //Zero out state variables
    for(int i=0; i<MAXCHANNELS; i++)
    {
        notePitch[i] = 0;
        noteVol[i]   = 0;
        notePhase[i] = 0;
        noteExpr[i] = 0.5;
        noteExprTarget[i] = noteExpr[i];
    }
    //Set the wave for the finger
    setExprMix();
}

static void renderNoise(long* dataL, long* dataR, unsigned long samples)
{
    for(int i=0; i<samples; i++)
    {
        dataL[i] = 0;
        dataR[i] = 0;
    }
    //Compute overvolume
    totalNoteVolume = 0;
    int fingersDown=0;
    for(int f=0; f<MAXCHANNELS; f++)
    {
        totalNoteVolume += noteVol[f];
        if(noteVol[f]>0)
        {
            fingersDown++;
        }
    }
    totalNoteVolume*=8;
    float scaleFinger=1;
    if(totalNoteVolume > 1)
    {
        scaleFinger = 1/totalNoteVolume;
    }
    //NSLog(@"scaleFinger %f ",scaleFinger);
    
    //Go in channel major order because we skip by volume
    for(int f=0; f<MAXCHANNELS; f++)
    {
        doRamp(&allFingers.finger[f].pitchRamp);
        doRamp(&allFingers.finger[f].volRamp);
        doRamp(&allFingers.finger[f].exprRamp);
        
        notePitch[f] = notePitchTarget[f] * 0.75 + notePitch[f] * 0.25;
        if(noteVol[f]==0)
        {
            notePhase[f] = 0;
            notePitch[f] = notePitchTarget[f];
        }
        if(noteVol[f] > 0 || noteVolTarget[f] > 0)
        {
            float p = notePhase[f];
            //note 33 is our center pitch, and it's 440hz
            float cyclesPerSample = powf(2,(notePitch[f]-33)/12) * (440/(44100.0 * 32));
            //computeNoteMixPerChannel(f);
            for(int i=0; i<samples; i++)
            {
                doRamp(&allFingers.finger[f].volRamp);
                doRamp(&allFingers.finger[f].exprRamp);
                noteExpr[f] = noteExprTarget[f] * 0.01 + noteExpr[f] * 0.99;
                float e = (noteExpr[f]);
                noteVol[f] = noteVolTarget[f] * 0.0001 + noteVol[f] * 0.9999;                    
                float v = noteVol[f];
                float cycles = i*cyclesPerSample + p;
                float cycleLocation = (cycles - (int)cycles); // 0 .. 1
                int j = (int)(cycleLocation*WAVEMAX);
                long s = INT_MAX/8 * v * 
                    ((waveMix[0][0][j]*e + waveMix[0][1][j]*(1-e)) * (scaleFinger)) +
                    ((waveMix[1][0][j]*e + waveMix[1][1][j]*(1-e)) * (1-scaleFinger));
                dataL[i] = s;
                dataR[i] = s;
            }     
            notePhase[f] = (cyclesPerSample*samples) + p;
        }
    }
    allFingers.sampleCount += samples;
}

void rawEngine(int midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr)
{
    //We limit to one note per midi channel now
    int channel = midiChannel;
    if(doNoteAttack)
    {
        //Set to beginning of sustain phase.
        //In the future, the attack and decase phase will have its own envelope, and this
        //will be how impulses, etc get handled.
        //notePhase[channel] = 0;
    }
    noteVolTarget[channel] = volVal;
    notePitchTarget[channel] = pitch;
    noteExprTarget[channel] = midiExpr/127.0;
    
    setRamp(&allFingers.finger[channel].volRamp, 4096, volVal);
    setRamp(&allFingers.finger[channel].pitchRamp, 4096, volVal);
    setRamp(&allFingers.finger[channel].exprRamp, 4096, volVal);
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
    NSLog(@"rawEngineStop");    
}



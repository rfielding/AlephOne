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

#define WAVEMAX (2048*2)
#define MAXCHANNELS 16
#define EXPRLEVELS 32
float notePitch[MAXCHANNELS];
float noteVol[MAXCHANNELS];
float notePitchTarget[MAXCHANNELS];
float noteVolTarget[MAXCHANNELS];
float notePhase[MAXCHANNELS];
int   noteExpr[MAXCHANNELS];
int   noteExprTarget[MAXCHANNELS];
float waveSustain[EXPRLEVELS][WAVEMAX];


/**
 We fade between two sets of harmonics based on expr
 */
#define HARMONICSMAX 32
float harmonicsLoTotal=1;
float harmonicsLo[HARMONICSMAX] =
{
    // 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32
       8, 4, 1, 2, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
float harmonicsHiTotal=1;
float harmonicsHi[HARMONICSMAX] =
{
    // 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32
       0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
float harmonicsTotal=1;
float harmonics[HARMONICSMAX];

/*
   We set two sets of harmonic content, and expr fades between the two of them.
 */
//exprLevel 0..EXPRLEVELS
static void setExprMixForLevel(int exprLevel)
{
    harmonicsLoTotal = 0;
    for(int i=0; i<HARMONICSMAX; i++)
    {
        harmonicsLoTotal += harmonicsLo[i];
    }
    
    harmonicsHiTotal = 0;
    for(int i=0; i<HARMONICSMAX; i++)
    {
        harmonicsHiTotal += harmonicsHi[i];
    }
    
    float mix = (1.0 * exprLevel) / EXPRLEVELS;
    
    //Mix together the current waveform
    float lowMix = (1-mix) / harmonicsLoTotal;
    float hiMix = (mix) / harmonicsHiTotal;
    
    //Note: sum(harmonics[h]) == 1
    for(int h=0; h<HARMONICSMAX; h++)
    {
        harmonics[h] = lowMix * harmonicsLo[h] + hiMix * harmonicsHi[h]; 
    }        
    
    for(int i=0; i<WAVEMAX;i++)
    {
        waveSustain[exprLevel][i] = 0;
    }       
    for(int h=0; h<HARMONICSMAX; h++)
    {
        if(harmonics[h] > 0)
        {
            for(int i=0; i<WAVEMAX;i++)
            {
                float sampleValue = harmonics[h]/MAXCHANNELS * sinf( ((h+1) * (2*M_PI) * i )/ WAVEMAX );
                waveSustain[exprLevel][i] += (int) (LONG_MAX * sampleValue);
            }
        }
    }
}

static void setExprMix()
{
    for(int i=0; i<EXPRLEVELS;i++)
    {
        setExprMixForLevel(i);
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
    //Zero out state variables
    for(int i=0; i<MAXCHANNELS; i++)
    {
        notePitch[i] = 0;
        noteVol[i]   = 0;
        notePhase[i] = 0;
        noteExpr[i] = EXPRLEVELS/2;
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
    //Go in channel major order because we skip by volume
    for(int f=0; f<MAXCHANNELS; f++)
    {
        noteVol[f] = noteVol[f] * 0.00001 + noteVolTarget[f] * 0.99999;
        notePitch[f] = notePitch[f] * 0.01 + notePitchTarget[f] * 0.99;
        if(noteExpr[f] < noteExprTarget[f])noteExpr[f]++;
        if(noteExpr[f] >= noteExprTarget[f])noteExpr[f]--;
        float v = noteVol[f];
        if(v > 0)
        {
            float p = notePhase[f];
            //note 33 is our center pitch, and it's 440hz
            float cyclesPerSample = powf(2,(notePitch[f]-33)/12) * (440/(44100.0 * 32));
            //If non-zero volume, then we must add in
            int expr = noteExpr[f];
            for(int i=0; i<samples; i++)
            {
                float cycles = i*cyclesPerSample + p;
                float cycleLocation = (cycles - (int)cycles); // 0 .. 1
                int j = (int)(cycleLocation*WAVEMAX);
                long s = v * waveSustain[expr][j];
                dataL[i] += v * s;
                dataR[i] += dataL[i];
            }     
            notePhase[f] = (cyclesPerSample*samples) + p;
        }
    }
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
    noteExprTarget[channel] = (int) ( (midiExpr/127.0) * EXPRLEVELS );
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



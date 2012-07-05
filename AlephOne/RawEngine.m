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
#import <MobileCoreServices/UTCoreTypes.h>
#import <AudioToolbox/CAFFile.h>
#import <CoreFoundation/CFByteOrder.h>

#import "RawEngine.h"
#import "RawEngineGenerated.h"
#include "FretlessCommon.h"
#include "Parameters.h"

#define LOOPBUFFERMAX (1024*1024)
#define ECHOBUFFERMAX (1024*128)
#define UNISONMAX 3
#define HARMONICSMAX 64
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
    //struct ramp pitchLocation;
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

// loopIndexBufferAt is the sample corresponding to loopBufferL[0]
//    [bufferAt] [loopIndexStartLoop] [loopIndexEndLoop] [loopIndexReleaseLoop] [loopIndexBufferOverflowAt]
float loopBufferL[LOOPBUFFERMAX] __attribute__ ((aligned));
float loopBufferR[LOOPBUFFERMAX] __attribute__ ((aligned));

struct {
    unsigned long idxRequest;
    unsigned long idxBuffer;
    unsigned long idxStartLoop;
    unsigned long idxEndLoop;
    unsigned long idxRelease;
    unsigned long idxOverflow;
    
    int size;
    int firstOffset;
    int secondOffset;
    
    float feeding;
    float level;
} loop;

//Stereo 16bit 44.1khz stereo goes here, with space for CAFF headers appended
char copyBuffer8[2*LOOPBUFFERMAX*2+1024];



static inline long floatToSample16(float f)
{
    float scaleFactor = 0x8000 * 2.0/M_PI * 2;    
    return scaleFactor * f;
}

void audioCopyWrite(char* buffer,int* cursorp,long val)
{
    buffer[*cursorp] = val;
    *cursorp += 4;
}

void audioCopyWrite8(char* buffer,int* cursorp,char val)
{
    buffer[*cursorp] = val;
    *cursorp += 1;
}

void audioCopyWrite16(char* buffer,int* cursorp,int16_t val)
{
    *((uint16_t*)(&buffer[*cursorp])) = CFSwapInt16HostToBig(val);
    *cursorp += 2;
}

void audioCopyWrite32(char* buffer,int* cursorp,int32_t val)
{
    *((uint32_t*)(&buffer[*cursorp])) = CFSwapInt32HostToBig(val);
    *cursorp += 4;
}

void audioCopyWrite64(char* buffer,int* cursorp,int64_t val)
{
    *((uint64_t*)(&buffer[*cursorp])) = CFSwapInt64HostToBig(val);
    *cursorp += 8;
}

void audioCopyWrite64f(char* buffer,int* cursorp,double_t val)
{
    *((double_t*)(&buffer[*cursorp])) = CFConvertDoubleHostToSwapped(val).v;
    *cursorp += 8;
}

void audioCopy()
{    
    UIPasteboard *board = [UIPasteboard generalPasteboard];
    
    // Write the straight loop in
    int cursor=0;
    NSLog(@"writing CAF format to UIPasteboard");
    NSLog(@"writing type,version,flags");
    //Type,Version,Flags
    audioCopyWrite32(copyBuffer8,&cursor,'caff'); //type of file (magic)    
    audioCopyWrite16(copyBuffer8,&cursor,1);      //version
    audioCopyWrite16(copyBuffer8,&cursor,0);      //flags
    //Chunk header, type, len
    audioCopyWrite32(copyBuffer8,&cursor,'desc'); //description header follows
    audioCopyWrite64(copyBuffer8,&cursor,32);     //32 to bytes required for it
    
    //Description 44100 in float bigendian - not sure why the simple thing didn't work,
    //but this *does* work.
    audioCopyWrite8(copyBuffer8,&cursor,0x40);
    audioCopyWrite8(copyBuffer8,&cursor,0xE5);
    audioCopyWrite8(copyBuffer8,&cursor,0x88);
    audioCopyWrite8(copyBuffer8,&cursor,0x80);
    audioCopyWrite8(copyBuffer8,&cursor,0x00);
    audioCopyWrite8(copyBuffer8,&cursor,0x00);
    audioCopyWrite8(copyBuffer8,&cursor,0x00);
    audioCopyWrite8(copyBuffer8,&cursor,0x00);
    
    audioCopyWrite32(copyBuffer8,&cursor,'lpcm');  //format
    audioCopyWrite32(copyBuffer8,&cursor,0);       //flags -- big endian
    audioCopyWrite32(copyBuffer8,&cursor,4);       //bytes per packet
    audioCopyWrite32(copyBuffer8,&cursor,1);       //frames per packet
    audioCopyWrite32(copyBuffer8,&cursor,2);       //channels
    audioCopyWrite32(copyBuffer8,&cursor,16);      //bits per channel
    
    NSLog(@"writing dataheader");
    audioCopyWrite32(copyBuffer8,&cursor,'data');       //data follows
    audioCopyWrite64(copyBuffer8,&cursor,loop.size*4);  //4 bytes per stereo sample
    
    for(int i=0;i<16*4;i++)
    {
        printf("%1.2x ",(unsigned char)copyBuffer8[i]);
    }
    
    NSLog(@"writing stereo 16bit 44.1hz stereo data of length %d",loop.size);
    for(int i=0; i<loop.size; i++)
    {
        //Is left or right first?  It will matter when I get paste to work on GB
        int16_t sampleL = floatToSample16(loopBufferL[i + loop.firstOffset + loop.secondOffset]);
        int16_t sampleR = floatToSample16(loopBufferR[i + loop.firstOffset + loop.secondOffset]);
        audioCopyWrite16(copyBuffer8,&cursor,sampleL);
        audioCopyWrite16(copyBuffer8,&cursor,sampleR);
    }
    
    //This actually causes Garage Band to try.. formatting just seems wrong
    NSLog(@"copy data to pasteboard");
    NSData *dataFile = [NSData dataWithBytes:copyBuffer8 length:cursor];    
    [board setData:dataFile forPasteboardType:(NSString*)kUTTypeAudio];
}

float echoBufferL[ECHOBUFFERMAX] __attribute__ ((aligned));
float echoBufferR[ECHOBUFFERMAX] __attribute__ ((aligned));

float octaveHarmonicLimit[OCTAVES] = {
  42,40,38,36,32,24,16,8,4,2,1,1    
};

float unisonDetune[UNISONMAX] = {
    0, -0.4, 0.4    
};
float unisonVol[UNISONMAX] = {
  1, 1, 1  
};

float harmonics[EXPR][DIST][HARMONICSMAX];

int reverbDataL[REVERBECHOES] __attribute__ ((aligned)) =
{
  3,7,13,19,29,37,43,49,53,59
};
int reverbDataR[REVERBECHOES] __attribute__ ((aligned)) =
{
  5,11,17,23,31,41,47,51,57,61
};

float reverbStrength[REVERBECHOES] __attribute__ ((aligned)) =
{
    0.99, 0.99, 0.98, 0.99, 0.99, 0.99, 0.99, 0.99,0.99,0.99
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

void loopReset()
{
    loop.idxRequest = 0;
    loop.idxBuffer = 0;
    loop.idxStartLoop = 0;
    loop.idxEndLoop = 0;
    loop.idxRelease = 0;
    loop.idxOverflow = 0;
    
    loop.size = 0;
    loop.firstOffset = 0;
    loop.secondOffset = 0;
    bzero(loopBufferL,sizeof(float)*LOOPBUFFERMAX);
    bzero(loopBufferR,sizeof(float)*LOOPBUFFERMAX);
}

void loopClear()
{
    loopReset();
    loop.feeding = 0;
    loop.level = 0;
}

void reNormalizeToMax(float* buffer)
{
    //Pass through wave and make max value 1.0
    float maxValue = 0.0;
    
    //Find max value
    for(int i=0; i<WAVEMAX; i++)
    {
        float val = buffer[i];
        if(fabs(val) > maxValue)
        {
            maxValue = fabs(val);
        }
    }
    //Divide everything by max value
    if(maxValue > 0)
    {
        for(int i=0; i<WAVEMAX; i++)
        {
            buffer[i] = buffer[i]/maxValue;
        }        
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
    
    loopClear();
    
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
        harmonics[1][0][2*i+1] = 1.0/(2*(i+1));
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
                int s = (squareHarmonic*2+1) * 1.0/(harmonic+1);
                int h = s*(harmonic+1);
                //1 3  5  7  9 11 13
                //2 6 10 14 18 22 26
                //3 9 15 21 27 33 39
                if(s<HARMONICSMAX && h+1 < HARMONICSMAX)
                {
                    harmonics[expr][1][h-1] += harmonics[expr][0][harmonic] / s;                                                                    
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
    for(int sample=0; sample<WAVEMAX; sample++)
    {
        _waveFundamental[sample] = sinf( 1 * sample * 2.0 * M_PI / WAVEMAX ) * 0.75;
    }
    
    //Make no sample exceed 1.0
    for(int oct=0; oct<OCTAVES; oct++)
    {
        for(int dist=0; dist<DIST; dist++)
        {
            for(int expr=0; expr<EXPR; expr++)
            {
                reNormalizeToMax(waveMix[oct][expr][dist]);
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
        //allFingers.finger[f].pitchLocation.value = allFingers.finger[f].pitchLocation.finalValue;
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



int loopRepeatState()
{
    return 0;
}

void loopRepeat()
{
    if(0 < loop.idxRequest && loop.idxRequest < loop.idxBuffer && 
       loop.idxBuffer < loop.idxOverflow &&
       allFingers.sampleCount < loop.idxOverflow)
    {
        //Don't change anything unless we are safe
        unsigned long endLoop  = allFingers.sampleCount;
        int totalOffset   = (loop.idxBuffer - loop.idxRequest);
        
        //Start and size need initial guesses
        unsigned long startLoop = loop.idxBuffer + totalOffset;
        int size     = (endLoop - startLoop);
        
        //If our guess makes size negative, then just pick the lowest value for startLoop
        if(size < 1.5*totalOffset)
        {
            startLoop = (loop.idxRequest + endLoop)/2;
            size = endLoop - startLoop;
        }
        
        //We are safe
        if(0 < size && size + 2*totalOffset < LOOPBUFFERMAX)
        {
            //Figure out the offset parts
            if(size < totalOffset)
            {
                loop.secondOffset = size;
                loop.firstOffset = totalOffset - loop.secondOffset;
            }
            else 
            {
                loop.secondOffset = totalOffset;
                loop.firstOffset = 0;
            }
            loop.size = size;
            
            loop.idxStartLoop = startLoop;
            //Set the actual loop end and release
            loop.idxEndLoop = endLoop;
            //Release lets us continue to write out data to prevent impulsing
            loop.idxRelease = loop.idxEndLoop + totalOffset;
            //Prevent us from coming back in with requests until we get another
            loop.idxRequest = 0;
            
            //Wrap the attack and release parts around
            for(int i=0; i < loop.secondOffset; i++)
            {
                int tailWrite = totalOffset + i;
                int tailRead =  tailWrite + loop.size;
                //Wrap releasing tail
                loopBufferL[tailWrite] += loopBufferL[tailRead];
                loopBufferR[tailWrite] += loopBufferR[tailRead];                    
                
                int attackRead  = i + loop.firstOffset;
                int attackWrite = attackRead + size;
                //Wrap attack
                loopBufferL[attackWrite] += loopBufferL[attackRead];
                loopBufferR[attackWrite] += loopBufferR[attackRead];                    
            }
        }
        else 
        {
            loopReset();
        }
    }
    else 
    {
        loop.idxRequest = allFingers.sampleCount;
    }
}

int loopCountInState()
{
    return 0;
}

void loopCountIn()
{
    if(0 < loop.idxRequest && loop.idxBuffer < loop.idxRequest)
    {
        //Clear everything but where we requested from
        unsigned long request = loop.idxRequest;
        loopReset();
        
        loop.idxRequest = request;
        loop.idxBuffer = allFingers.sampleCount;
        loop.idxOverflow = loop.idxBuffer + LOOPBUFFERMAX;   
    }
    else 
    {
        loopReset();
    }
}



void setLoopFeed(float val)
{
    loop.feeding = val;
}

float getLoopFeed()
{
    return loop.feeding;
}

void setLoopFade(float val)
{
    loop.level = val;
}

float getLoopFade()
{
    return loop.level;
}

//This is a feed-forward echo
static inline void renderEcho(const int n,const int n2)
{
    echoBufferL[n2] += echoBufferL[n];
    echoBufferR[n2] += echoBufferR[n];
    echoBufferL[n2] *= 0.475;
    echoBufferR[n2] *= 0.475;
    echoBufferL[n] = 0;
    echoBufferR[n] = 0;    
}

//This is a feed-forward reverb, a convolution hack
static inline void renderConvolution(
    const int i,const int sc,
    const float channelBleed,
    const float totalL, const float totalR)
{
    const int sci = i+sc;
    //Should unroll because of the constants
    for(int r=0; r<REVERBECHOES; r++)
    {
        const int nL = sci+reverbDataL[r];
        const int nR = sci+reverbDataR[r];
        const float vL = totalL*reverbStrength[r];
        const float vR = totalR*reverbStrength[r];
        const int nLX = nL % ECHOBUFFERMAX;
        const int nRX = nR % ECHOBUFFERMAX;
        echoBufferL[nLX] += vL + channelBleed*vR;
        echoBufferR[nRX] += vR + channelBleed*vL;
    }    
}

static inline float renderSumChorus(
    const int i, 
    const float innerScale,
    const float dist, const float noDist)
{
    float rawTotal = 0;
    //Sum up all fingers per chorus voice - should unroll because of the constants
    for(int phaseIdx=0; phaseIdx<UNISONMAX; phaseIdx++)
    {
        const float raw = allFingers.total[phaseIdx][i];
        //Is the atanf bad?
        rawTotal += dist*compress(raw * innerScale) + 2*noDist*raw;
    }     
    return rawTotal;
}

static inline void renderGetLoopFeed(
    const unsigned long now,
    const float dying,
    float* lLp,float* lRp)
{
    //Read from the looper into our audio
    const int offset = loop.firstOffset + loop.secondOffset;
    const int loopIdx = offset + (now - loop.idxBuffer - offset)%loop.size;
    loopBufferL[loopIdx] *= (1-dying);
    loopBufferR[loopIdx] *= (1-dying);
    *lLp = loopBufferL[ loopIdx ];
    *lRp = loopBufferR[ loopIdx ];
}

static inline void renderCompression(
    const float finalScale,
    const float scaledTotal,
    const float reverbBoost,const float noReverbAmount,
    const float lL,const float lR,
    const float feedRawL,const float feedRawR,
    float* aLp, float* aRp, float* aLRawp,float* aRRawp)
{
    *aLp = atanf(finalScale * (reverbBoost*feedRawL + scaledTotal*noReverbAmount + lL));
    *aRp = atanf(finalScale * (reverbBoost*feedRawR + scaledTotal*noReverbAmount + lR));
    *aLRawp = atanf(finalScale * (reverbBoost*feedRawL + scaledTotal*noReverbAmount));
    *aRRawp = atanf(finalScale * (reverbBoost*feedRawR + scaledTotal*noReverbAmount));    
}

#define RF_SCALEFACTOR (((long)0x2000000) / (M_PI/2))
#define RF_ISCALEFACTOR (0.75)

static inline void renderFinalizeBuffer(
        long* dataL,long* dataR,
        const int i,
        const float aL,const float aR)
{
    dataL[i] = (long) (RF_SCALEFACTOR * atanf(aL * RF_ISCALEFACTOR));
    dataR[i] = (long) (RF_SCALEFACTOR * atanf(aR * RF_ISCALEFACTOR));                
}

static inline void renderUpdateLoopBuffer(
    const unsigned long now,
    const float feeding,const float dying,
    const float aLRaw,const float aRRaw)
{
    const int isLooping = (0 < loop.size);
    const int isLoopRecording = (loop.size==0 && 0 < loop.idxBuffer);    
    const int isRecording = (isLoopRecording && loop.idxBuffer <= now) || (isLooping && loop.idxEndLoop <= now && now < loop.idxRelease);
    const int isNotOverflowed = (now < loop.idxOverflow);
    
    //We are recording
    if(isRecording)
    {
        if(isNotOverflowed)
        {
            const int loopIdx = (now - loop.idxBuffer);
            loopBufferL[loopIdx] = aLRaw;
            loopBufferR[loopIdx] = aRRaw;                
        }
    }
    else 
    {
        if(isLooping)
        {
            const int offset = loop.firstOffset + loop.secondOffset;
            const int loopIdx = offset + (now - loop.idxBuffer - offset)%loop.size;
            const float notDying = (1-dying);
            loopBufferL[loopIdx] = notDying*(loopBufferL[loopIdx] + aLRaw*feeding);
            loopBufferR[loopIdx] = notDying*(loopBufferR[loopIdx] + aRRaw*feeding);
        }
    }    
}

/**
   Make as much as possible constant, and force loops to unroll.
   After enough refactoring, it may become apparent how to vDSP-ize this code.
 */
static inline void renderLoopIteration(
    long* dataL,long* dataR,
    const int i,const int sc,
    const float dying,
    const float innerScale,
    const float dist,const float noDist,
    const float reverbAmount,const float noReverbAmount,
    const float feeding)
{
    //Read from the looper into our audio
    //loopSize is only non-zero when all other values are checked and set correctly
    const unsigned long now = allFingers.sampleCount + i;
    const int isLooping = (0 < loop.size);
    
    //Feed in the loop if we have to
    float lL=0;
    float lR=0;     
    if(isLooping)
    {
        renderGetLoopFeed(now, dying, &lL, &lR);        
    }
    const float rawTotal = renderSumChorus(i, innerScale, dist, noDist);
    
    const int n = (i+sc)%ECHOBUFFERMAX;
    const int n2 = (i+1+sc)%ECHOBUFFERMAX;
    const float feedL = echoBufferL[n];
    const float feedR = echoBufferR[n];
    const float reverbBoost = 2.1;
    const float totalScale = 0.25;
    const float feedScale = 0.1;
    const float channelBleed = 0.125;
    const float finalScale = 0.75;
    const float scaledTotal = rawTotal*totalScale;
    const float feedRawL = feedL*feedScale*reverbAmount;
    const float feedRawR = feedR*feedScale*reverbAmount;
    const float totalL = feedRawL + scaledTotal;
    const float totalR = feedRawR + scaledTotal;    
    renderEcho(n,n2);
    renderConvolution(i,sc,channelBleed,totalL,totalR);
    
    float aL;
    float aR;
    float aLRaw;
    float aRRaw;
    renderCompression(finalScale,scaledTotal,reverbBoost,noReverbAmount,lL,lR,feedRawL,feedRawR,&aL,&aR,&aLRaw,&aRRaw);
    renderUpdateLoopBuffer(now,feeding,dying,aLRaw,aRRaw);  
    renderFinalizeBuffer(dataL,dataR,i,aL,aR);
}

static inline void renderNoiseToBuffer(long* dataL, long* dataR, unsigned long samples)
{
    float dist = (allFingers.distRamp.value);
    float noDist = 1 - dist;
    dist=dist*dist;
    float innerScale = (0.5+10.5*dist) * 0.5;
    
    //Scale to fit range when converting to integer
    
    long sc = allFingers.sampleCount;
    float reverbAmount = (allFingers.reverbRamp.value * allFingers.reverbRamp.value);
    float noReverbAmount = (1 - reverbAmount);
    reverbAmount = reverbAmount*0.95;
    
    float dying = (1-loop.feeding)*loop.level;
    float feeding = loop.feeding*loop.level;
    //This whole part of the chain only happens once, not proportional to number of fingers or chorus voices
    //Add pre-chorus sound together compressed
    for(int i=0; i<samples; i++)
    {
        renderLoopIteration(dataL,dataR,i,sc,dying,innerScale,dist,noDist,reverbAmount,noReverbAmount,feeding);    
    }    
}


static void renderNoiseInnerLoop(int f,unsigned long samples,float invSamples,
                                        float currentVolume,float diffVolume, float currentExpr, float diffExpr)
{
    float notep = allFingers.finger[f].pitchRamp.value;
    float currentTimbre = allFingers.timbreRamp.value;
    float targetTimbre = allFingers.timbreRamp.stopValue;
    float deltaTimbre = (targetTimbre - currentTimbre)/samples;
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
                                           currentTimbre,deltaTimbre,phase,
                                           samples,invSamples,
                                           currentVolume*uVol,diffVolume*invSamples*uVol,
                                           currentExpr,diffExpr*invSamples);
    }
}

static void renderNoise(long* dataL, long* dataR, unsigned long samples)
{
    renderNoiseCleanAll(samples);
    int activeFingers=0;
    float invSamples = 1.0/samples;
    
    setRamp(&allFingers.reverbRamp, 64, getReverb());
    setRamp(&allFingers.detuneRamp, 64, getDetune());
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
            int rampVal = 2;
            if(volVal == 0)
            {
                rampVal+=2;
            }
            setRamp(&allFingers.finger[channel].volRamp, rampVal, volVal);            
        }
        if(volVal!=0) //Don't bother with ramping these on release
        {
            setRamp(&allFingers.finger[channel].pitchRamp, 1, pitch);
            setRamp(&allFingers.finger[channel].exprRamp, 3, midiExpr/127.0);                                            
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
            
            NSLog(@"%fhz %luchannel %lubit",audioFormat.mSampleRate,audioFormat.mChannelsPerFrame,audioFormat.mBitsPerChannel);
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



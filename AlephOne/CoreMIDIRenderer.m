//
//  CoreMIDIRenderer.m
//  AlephOne
//
//  Created by Robert Fielding on 11/27/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


#import <CoreMIDI/CoreMIDI.h>
#import "CoreMIDIRenderer.h"
#import <stdarg.h>


#define MIDIBUFFERSIZE 1024
static MIDIPortRef midiOutPort = 0;
static MIDIClientRef midiClient = 0;
static Byte midiBuffer[MIDIBUFFERSIZE];
static Byte midiSendBuffer[MIDIBUFFERSIZE];
static int midiBufferCount = 0;
static struct Fretless_context* fretlessp = NULL;

static int logFromLastPassed=0;
static Byte logBuffer[MIDIBUFFERSIZE];

static void CoreMIDIRenderer_midiStateChangedHander(const MIDINotification *message, void *refCon)
{
    NSLog(@"midiState changed\n");
}

void CoreMIDIRenderer_midiInit(struct Fretless_context* ctxp)
{
    if(midiOutPort)return;
    for(int i=0;i<MIDIBUFFERSIZE;i++)
    {
        midiBuffer[i] = 0x00;
        midiSendBuffer[i] = 0x00;
    }
    fretlessp = ctxp;
    midiBufferCount = 0;
    OSStatus status = 0;
    
    status = MIDIClientCreate(CFSTR("AlephOne client"),CoreMIDIRenderer_midiStateChangedHander,NULL,&midiClient);
    if(midiClient != NULL && status == 0)
    {
        status = MIDIOutputPortCreate(midiClient,CFSTR("AlephOne out port"),&midiOutPort);
        if(midiOutPort == NULL || status != 0)
        {
            NSLog(@"Failed to allocate midi out port");
        }
        if(status == 0 && midiClient && midiOutPort)
        {
            NSLog(@"MIDI appears to be setup\n");
        }
    }
    else
    {
        NSLog(@"Failed to allocate midi client");
    }
    [MIDINetworkSession defaultSession].enabled = TRUE;
}

void CoreMIDIRenderer_midiPutch(char c)
{
    if(midiBufferCount + 1 >= MIDIBUFFERSIZE)
    {
        NSLog(@"refusing to overflow midi buffer in midiPutch");
        return;
    }
    //overflows possible!
    midiBuffer[midiBufferCount] = c;
    midiBufferCount++;
    
    //Keep a record of the message
    logBuffer[logFromLastPassed % MIDIBUFFERSIZE] = c;
    logFromLastPassed++;
}

void CoreMIDIRenderer_midiFlush()
{
    if(midiBufferCount > 0)
    {
        if(midiOutPort)
        {            
            MIDIPacketList* pktList = (MIDIPacketList*)midiSendBuffer;
            MIDIPacket* curPacket = MIDIPacketListInit(pktList);
            MIDIPacket* sentPacket = MIDIPacketListAdd(pktList, 
                                                       sizeof(midiSendBuffer), 
                                                       curPacket, 0, 
                                                       midiBufferCount, midiBuffer
                                                       );
            if(sentPacket == NULL)
            {
                NSLog(@"midi data wasn't sent!\n");
            }
            // Send the backet to all destinations that are flagged to receive it.
            ItemCount nDests = MIDIGetNumberOfDestinations();
            for(unsigned int i=0;i<nDests;i++)
            {
                MIDIEndpointRef dest = MIDIGetDestination(i);
                //printf("\nflush %d bytes to %d\n",midiBufferCount,i);
                MIDISend(midiOutPort, dest, pktList);
            }
        }
    }
    midiBufferCount = 0;    
}

void CoreMIDIRenderer_midiPassed()
{
    //Restart log (should happen every time fingers come up!)
    logFromLastPassed = 0;
}

int CoreMIDIRenderer_midiFail(const char* msg,...)
{
    va_list argp;
    va_start(argp,msg);
    vfprintf(stderr,msg,argp);
    va_end(argp);
    
    if(logFromLastPassed > 0)
    {
        //Dump the MIDI log
        if(logFromLastPassed < MIDIBUFFERSIZE)
        {
            //This is the simple case, from zero until crash time
            for(int i=0; i<logFromLastPassed; i++)
            {
                //Line break on bytes with high bit set
                if(logBuffer[i] & 0x80)
                {
                    printf("\n");
                }
                printf("%2x:",logBuffer[i]);
            }
            printf("\n");
        }
        else
        {
            //It's a large, truncated log
            printf("...\n");
            for(int i=0; i<MIDIBUFFERSIZE; i++)
            {
                //Line break on bytes with high bit set
                int n = (i+logFromLastPassed)%MIDIBUFFERSIZE;
                if(logBuffer[n] & 0x80)
                {
                    printf("\n");
                }
                printf("%2x:",logBuffer[n]);
            }
            printf("\n");
        }
    }
    return 0;
}




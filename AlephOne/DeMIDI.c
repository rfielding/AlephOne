//
//  DeMIDI.c
//  AlephOne
//
//  Created by Robert Fielding on 2/1/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//
/**
    We are only handling one note per channel, since our instrument is designed
    to spread across 16 channels anyway.  This dramatically simplifies the engine.
 */

#include "DeMIDI.h"
#include <stdio.h>

#define BUFFERMAX 2048
static unsigned char buffer[BUFFERMAX];
static int bufferIdx = 0;

//This state is used to forward pitches to the audio engine
static unsigned char  midiStatus;
static unsigned char  midiChannel;
static unsigned char  midiVol;
static unsigned char  midiNote;
static int   midiBend;
static unsigned char  intLow;
static unsigned char  intHi;
static int   midiExpr;
static int   midiExprParm; //??? expression isn't a single item, it's a parm with a value
static int   midiPitchBendSemis = 2;
static int   doNoteAttack;
static float midiPitch;
static float volVal;
static float exprVal;
static unsigned char coarse;
static unsigned char fine;
static unsigned char rpnVal;
static unsigned char rpnFoo; //Not sure what this is

static void (*rawEngine)(char midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr);


void DeMIDI_start(void (*rawEngineArg)(char midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr))
{
    rawEngine = rawEngineArg;
    //Start the sound engine
}

void DeMIDI_stop()
{
    //Stop the sound engine
}

void DeMIDI_putch(char c)
{
    //Get a byte into the engine
    //We are a char driven FSM to turn bytes into pitch and expression control
    buffer[bufferIdx] = c;
    bufferIdx++;
}

void DeMIDI_flush()
{
    int dataByte = 0;
    //Decode buffer bytes into channel/bend, etc
    while(dataByte < bufferIdx)
    {
        int somethingChanged = 0;
        doNoteAttack = 0;
        
        //Picking out status byte is done separately to handle 
        //running status.
        if(0x80 & buffer[dataByte])
        {
            midiStatus = (buffer[dataByte] & 0xF0) >> 4;
            midiChannel = (buffer[dataByte] & 0x0F);
            //printf("status:%d %d\n",(int)midiStatus,(int)midiChannel);
            dataByte++;
        }      
        
        if(midiStatus == 0x09)
        {
            if(buffer[dataByte] & 0x80)printf("bad byte in 0x09 note\n");
            if(buffer[dataByte+1] & 0x80)printf("bad byte in 0x09 vol\n");
            midiNote = (buffer[dataByte] & 0x7F);
            midiVol  = (buffer[dataByte+1] & 0x7F);
            doNoteAttack = (midiVol != 0);  //TODO: can be modified by note tie!
            dataByte+=2;
            somethingChanged = 1;
        }
        else
        if(midiStatus == 0x08)
        {
            if(buffer[dataByte] & 0x80)printf("bad byte in 0x08 note\n");
            if(buffer[dataByte+1] & 0x80)printf("bad byte in 0x08 vol\n");
            midiNote = (buffer[dataByte] & 0x7F);
            midiVol  = 0;  //Ignore its value
            dataByte+=2;
            somethingChanged = 1;
        }
        else
        if(midiStatus == 0x0E)
        {
            if(buffer[dataByte] & 0x80)printf("bad byte in 0x0E bend low\n");
            if(buffer[dataByte+1] & 0x80)printf("bad byte in 0x0E bend high\n");
            intLow = buffer[dataByte] & 0x7F;
            intHi = buffer[dataByte+1] & 0x7F;
            //14bit MIDI bend
            midiBend = (int)(intHi * (1<<7)) + intLow;
            dataByte+=2;
            somethingChanged = 1;
        }
        else
        if(midiStatus == 0x0D)
        {
            if(buffer[dataByte] & 0x80)printf("bad byte in 0x0D chan press data\n");
            //Treating this as a volume update
            midiVol = buffer[dataByte] & 0x7F;
            //midiExpr = buffer[dataByte] & 0x7F;
            //midiExprParm = 0;
            dataByte+=1;
            somethingChanged = 1;
        }        
        else
        if(midiStatus == 0x0B)
        {
            if(buffer[dataByte] & 0x80)printf("bad byte in 0x0B parm low\n");
            if(buffer[dataByte+1] & 0x80)printf("bad byte in 0x0B  parm high\n");
            intLow = buffer[dataByte] & 0x7F;
            intHi = buffer[dataByte+1] & 0x7F;
            if(intLow == 0x63)
            {
                coarse = intLow;
            }
            else
            if(intLow == 0x62)
            {
                fine = intLow;
            }
            else
            if(intLow == 101)
            {
                coarse = intLow;
            }
            else
            if(intLow == 100)
            {
                fine = intLow;
            }            
            else
            if(intLow == 6)
            {
                rpnVal = intHi;
                somethingChanged = 1;
            }
            else
            if(intLow == 38)
            {
                rpnFoo = intHi;
            }            
            else
            if(intLow == 11)
            {
                midiExprParm = intLow;
                midiExpr = intHi;
                somethingChanged = 1;
            }
            //Just ignore status bytes for now
            dataByte+=2;
        }
        else
        {
            //Skip until status because we are lost
            int gotLost = 0;
            while(dataByte < bufferIdx && (buffer[dataByte] & 0x80)==0)
            {
                dataByte++;
                gotLost=1;
            }
            if(gotLost)
            {
                printf("we got lost in midi parsing on %d!\n", (int)midiStatus);
            }
        }
        
        if(somethingChanged)
        {
            if(midiStatus == 0x09 || midiStatus == 0x0E)
            {
                midiPitch = 1.0 * midiNote + (midiPitchBendSemis*(midiBend - 8192))/8192.0;
            }
            if(midiStatus == 0x09 || midiStatus == 0x08 || midiStatus == 0x0D)
            {
                volVal = midiVol / 127.0;            
            }
            if(midiStatus == 0x0B)
            {
                if(intLow == 6 && coarse == 101 && fine == 100)
                {
                    midiPitchBendSemis = rpnVal;
                }
            }
            //printf("rawEngine(%d,%d,%f,%f,%d,%d)\n",(int)midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
            rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
        }        
    }
    //TODO: do something with our pitch information
    //Done flushing
    bufferIdx = 0;
}
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
static char buffer[BUFFERMAX];
static int bufferIdx = 0;

//This state is used to forward pitches to the audio engine
static char  midiStatus;
static char  midiChannel;
static char  midiVol;
static char  midiNote;
static int   midiBend;
static char  intLow;
static char  intHi;
static int   midiExpr;
static int   midiExprParm; //??? expression isn't a single item, it's a parm with a value
static int   midiPitchBendSemis = 2;
static int   doNoteAttack;
static float midiPitch;
static float volVal;
static float exprVal;
static char coarse;
static char fine;
static char rpnVal;
static char rpnFoo; //Not sure what this is

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
            dataByte++;
        }      
        
        if(midiStatus == 0x09)
        {
            midiNote = (buffer[dataByte] & 0x7F);
            midiVol  = (buffer[dataByte+1] & 0x7F);
            doNoteAttack = (midiVol != 0);  //TODO: can be modified by note tie!
            dataByte+=2;
            somethingChanged = 1;
        }
        else
        if(midiStatus == 0x08)
        {
            midiNote = (buffer[dataByte] & 0x7F);
            midiVol  = 0;  //Ignore its value
            dataByte+=2;
            somethingChanged = 1;
        }
        else
        if(midiStatus == 0x0E)
        {
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
            //Treating this as a volume update
            midiVol = buffer[dataByte] & 0x7F;
            dataByte+=1;
            somethingChanged = 1;
        }        
        else
        if(midiStatus == 0x0B)
        {
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
            printf("rawEngine(%d,%d,%f,%f,%d,%d)\n",(int)midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
            rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
        }        
    }
    //TODO: do something with our pitch information
    //Done flushing
    bufferIdx = 0;
}
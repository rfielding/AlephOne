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

#define BUFFERMAX 256
static char buffer[BUFFERMAX];
static int bufferIdx = 0;

//This state is used to forward pitches to the audio engine
char  midiStatus;
char  midiChannel;
char  midiVol;
char  midiNote;
int   midiBend;
char  intLow;
char  intHi;
int   midiExpr;
int   midiChannelPress;
int   midiPitchBendSemis = 2;
int   doNoteAttack;
float midiPitch;
float volVal;
float exprVal;

//We have 16 note polyphony available
void UpdateEngine()
{
    //doNoteAttack,pitch,volVal,exprVal,midiChannel,expr should be sufficient.
}

void DeMIDI_start()
{
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
            doNoteAttack = 1;  //TODO: can be modified by note tie!
            dataByte+=2;
        }
        else
        if(midiStatus == 0x08)
        {
            midiNote = (buffer[dataByte] & 0x7F);
            midiVol  = 0;  //Ignore its value
            dataByte+=2;
        }
        else
        if(midiStatus == 0x0E)
        {
            intLow = buffer[dataByte] & 0x7F;
            intHi = buffer[dataByte+1] & 0x7F;
            //14bit MIDI bend
            midiBend = (int)(intHi * (1<<7)) + intLow;
            dataByte+=2;
        }
        else
        if(midiStatus == 0x0D)
        {
            //A rare 2 byte message.   Essentially updates volume(?) Beware!!
            //If we get lost during decode, that ends up creating stuff like stuck notes!
            midiChannelPress = buffer[dataByte] & 0x7F;
            dataByte+=1;
        }        
        
        //These can update pitch
        if(midiStatus == 0x09 || midiStatus == 0x0E)
        {
            midiPitch = 1.0 * midiNote + (midiPitchBendSemis*(midiBend - 8192))/8192.0;
        }
        if(midiStatus == 0x09 || midiStatus == 0x08)
        {
            volVal = midiVol / 127.0;            
        }
        UpdateEngine();        
    }
    //TODO: do something with our pitch information
    //Done flushing
    bufferIdx = 0;
}
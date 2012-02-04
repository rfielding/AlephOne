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

#define CHANNELMAX 16

static void (*rawEngine)(int midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr);


void DeMIDI_start(void (*rawEngineArg)(int midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr))
{
    rawEngine = rawEngineArg;
    //Start the sound engine
}

void DeMIDI_stop()
{
    //Stop the sound engine
}

/*
void DeMIDI_putch(char c)
{
    //Get a byte into the engine
    //We are a char driven FSM to turn bytes into pitch and expression control
    buffer[bufferIdx] = c;
    bufferIdx++;
}
 */

#define S_EXPECT_STATUS 0
#define S_ON_BYTE_NOTE 1
#define S_ON_BYTE_VOL 2
#define S_OFF_BYTE_NOTE 3
#define S_OFF_BYTE_VOL 4
#define S_BEND_LO 5
#define S_BEND_HI 6
#define S_RPN_LO 7
#define S_RPN_HI 8
#define S_NRPN_LO_KEY 9
#define S_NRPN_HI_KEY 10
#define S_RPN_VAL 11
#define S_RPN_LO_KEY 12
#define S_RPN_HI_KEY 13
#define S_CH_PRESS 14
#define S_RPN_11 15

int expectState=S_EXPECT_STATUS;

int midiStatus = 0;
int midiChannel = 0;
int expectDataBytes = 0;
int midiNote = 0;
int midiVol[CHANNELMAX];
int doNoteAttack = 0;
float midiPitch = 0;
float volVal = 0;
int midiExprParm = 0;
int midiExpr = 0;
int midiPitchBendSemis = 2;
int midiBend = 8192;
int nrpnKeyLo;
int nrpnKeyHi;
int rpnKeyLo;
int rpnKeyHi;
int rpnVal;
int expectNoteTie = 0;
//Handle ambiguity if 6 and 38 in nrpn setting here.
int isRegistered = 0;

void computePitch()
{
    midiPitch = 1.0 * midiNote + (midiPitchBendSemis*(midiBend - 8192))/8192.0;    
}

void computeVol(int channel)
{
    volVal = midiVol[channel] / 127.0;                
}

/**
 Just do the decode as an FSM
 The nrpn/rpn stuff is just nuts...
 */
void DeMIDI_putch(char c)
{    
    //Handle status bytes to get overall state
    if( c & 0x80 )
    {
        midiStatus = (c & 0x00F0) >> 4;
        midiChannel = (c & 0x000F);
        switch( midiStatus )
        {
            case 0x08:
                expectState = S_OFF_BYTE_NOTE;
                return;
            case 0x09:
                expectState = S_ON_BYTE_NOTE;
                return;
            case 0x0B:
                expectState = S_RPN_LO;
                return;
            case 0x0D:
                expectState = S_CH_PRESS;
                return;
            case 0x0E:
                expectState = S_BEND_LO;
                return;
            default:
                printf("we don't recognize status byte %d yet.\n",midiStatus);
                return;
        }
    }
    else
    {
        //Handle data bytes
        switch( expectState)
        {
            case S_ON_BYTE_NOTE:
                midiNote = (int)(c & 0x7F);
                expectState = S_ON_BYTE_VOL;
                return;
            case S_ON_BYTE_VOL:
                midiVol[midiChannel] = (int)(c & 0x7F);
                expectState = S_ON_BYTE_NOTE;
                computePitch();
                computeVol(midiChannel);
                //printf("v %d:%d:%d\n",midiChannel,midiNote[midiChannel],midiVol[midiChannel]);
                rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
                return;
                
            case S_OFF_BYTE_NOTE:
                midiNote = (int)(c & 0x7F);
                expectState = S_OFF_BYTE_NOTE;
                return;
            case S_OFF_BYTE_VOL:
                midiVol[midiChannel] = 0;
                expectState = S_OFF_BYTE_NOTE;
                volVal = 0;
                //printf("v %d:%d:%d\n",midiChannel,midiNote[midiChannel],midiVol[midiChannel]);
                rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
                return;
                
            case S_BEND_LO:
                midiBend = (int)(c & 0x7F);
                expectState = S_BEND_HI;
                return;
            case S_BEND_HI:
                midiBend = (((int)(c & 0x7F))<<7) + midiBend;
                expectState = S_BEND_LO;
                computePitch();
                computeVol(midiChannel);
                //printf("b %d:%d:%d\n",midiChannel,midiNote[midiChannel],midiVol[midiChannel]);
                rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
                return;
            
            case S_RPN_LO:
                {
                    switch((int)(c & 0x7F))
                    {
                        case 0x63:
                            expectState = S_NRPN_LO_KEY;
                            return;
                        case 0x62:
                            expectState = S_NRPN_HI_KEY;
                            return;
                        case 101:
                            expectState = S_RPN_LO_KEY;
                            return;
                        case 100:
                            expectState = S_RPN_HI_KEY;
                            return;
                        case 0x06:
                            expectState = S_RPN_VAL;
                            return;
                        case 11:
                            expectState = S_RPN_11;
                        return;
                    }                
                }
                return;
            case S_NRPN_LO_KEY:
                isRegistered = 0;
                nrpnKeyLo = (int)(c & 0x7F);
                return;
            case S_NRPN_HI_KEY:
                isRegistered = 0;
                nrpnKeyHi = (int)(c & 0x7F);
                return;
            case S_RPN_VAL:
                rpnVal = (int)(c & 0x7F);
                if(isRegistered && rpnKeyLo == 0 && rpnKeyHi == 0)
                {
                    midiPitchBendSemis = rpnVal;
                }
                else
                if(isRegistered==0 && rpnKeyLo == 9 && rpnKeyHi == 71)
                {
                    //Next on/off pair should be tied together.
                    expectNoteTie = 1;
                }
                return;
                
            case S_CH_PRESS:
                midiVol[midiChannel] = (int)(c & 0x7F);
                computeVol(midiChannel);
                rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
                return;
                
            case S_RPN_LO_KEY:
                isRegistered = 1;
                rpnKeyLo = (int)(c & 0x7F);
                return;
            case S_RPN_HI_KEY:
                isRegistered = 1;
                rpnKeyHi = (int)(c & 0x7F);
                return;
            case S_RPN_11:
                midiExprParm = 11;
                midiExpr = (int)(c & 0x7F);
                computeVol(midiChannel);
                rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
                return;
                
                
            case S_EXPECT_STATUS:
                printf("illegal state.\n we didn't start with status byte!\n");
                return;
            default:
                printf("skipping unrecognized data bytes in status %d\n", midiStatus);
                //Skip data bytes that we were not expecting
                return;
                
        }
    }
}


void DeMIDI_flush()
{
    //We don't do anything with data boundaries right now
}

/*
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
            printf("rawEngine(%d,%d,%f,%f,%d,%d),%d\n",(int)midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr,midiNote);
            rawEngine(midiChannel,doNoteAttack,midiPitch,volVal,midiExprParm,midiExpr);
        }        
    }
    //TODO: do something with our pitch information
    //Done flushing
    bufferIdx = 0;
}
*/
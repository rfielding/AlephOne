//
//  RawEngine.h
//  AlephOne
//
//  Created by Robert Fielding on 2/2/12.
//  Copyright (c) 2012 Rob Fielding Software.
//


void rawEngine(int midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr);
void rawEngineStart();
void rawEngineStop();

int loopCountInState();
void loopCountIn();

int loopRepeatState();
void loopRepeat();

float getLoopFade();
void setLoopFade(float val);

float getLoopFeed();
void setLoopFeed(float val);

void audioCopy();
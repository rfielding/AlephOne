//
//  RawEngine.h
//  AlephOne
//
//  Created by Robert Fielding on 2/2/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//



void rawEngine(int midiChannel,int doNoteAttack,float pitch,float volVal,int midiExprParm,int midiExpr);
void rawEngineStart();
void rawEngineStop();

void loopCountIn();
void loopRepeat();

float getLoopFade();
void setLoopFade(float val);

float getLoopFeed();
void setLoopFeed(float val);

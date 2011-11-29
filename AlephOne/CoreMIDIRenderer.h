//
//  CoreMIDIRenderer.h
//  AlephOne
//
//  Created by Robert Fielding on 11/27/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Fretless.h"

void midiInit(struct Fretless_context* fretlessp);
void midiPutch(char c);
void midiFlush();
int midiFail(const char* msg,...);
void midiPassed();

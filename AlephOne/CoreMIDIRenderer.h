//
//  CoreMIDIRenderer.h
//  AlephOne
//
//  Created by Robert Fielding on 11/27/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "Fretless.h"

void CoreMIDIRenderer_midiInit(struct Fretless_context* fretlessp);
void CoreMIDIRenderer_midiPutch(char c);
void CoreMIDIRenderer_midiFlush();
int CoreMIDIRenderer_midiFail(const char* msg,...);
void CoreMIDIRenderer_midiPassed();

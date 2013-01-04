//
//  CoreMIDIRenderer.h
//  AlephOne
//
//  Created by Robert Fielding on 11/27/11.
//  Copyright 2011 Rob Fielding Software.
//

/*  
  The putch/flush is all that is required for a synth to be embeddable in our API.
  An interface with these functions on a synth is sufficient to embed it into this app.
 */

struct Fretless_context;

void CoreMIDIRenderer_midiInit(struct Fretless_context* fretlessp);
void CoreMIDIRenderer_midiPutch(char c);
void CoreMIDIRenderer_midiFlush();
int CoreMIDIRenderer_midiFail(const char* msg,...);
void CoreMIDIRenderer_midiPassed();

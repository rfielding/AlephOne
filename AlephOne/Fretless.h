//
//  Fretless.h
//  AlephOne
//
//  Created by Robert Fielding on 10/14/11.
//
// This should remain a *pure* C library with no references to external libraries

struct Fretless_context;

////Phase1: Using this API is to invoke these to wire in the library
//Inject dependencies on CoreMIDI (etc) like this
struct Fretless_context* Fretless_init(
                                       void (*midiPutch)(char),void (*midiFlush)(), 
                                       void* (*fretlessAlloc)(unsigned long), 
                                       int (*fail)(const char*,...), 
                                       void (*passed)(),
                                       int (*logger)(const char*,...)
                                       );

//Phase2: Then you can set the hints (you should give them initial values now, and you can set these again as the library runs)
void Fretless_setMidiHintChannelBase(struct Fretless_context* ctxp, int base);
void Fretless_setMidiHintChannelSpan(struct Fretless_context* ctxp, int span);
void Fretless_setMidiHintChannelBendSemis(struct Fretless_context* ctxp, int semitones);
void Fretless_setMidiHintSupressBends(struct Fretless_context* ctxp, int supressBends);

//Phase3: Call this before calling down,express,move,up,flush,selfTest
void Fretless_boot(struct Fretless_context* ctxp);

//Must call this (per finger) before others are callable
void Fretless_down(struct Fretless_context* ctxp, int finger,float fnote,int polyGroup,float velocity,int legato);
//Callable for down or move, before flush
void Fretless_express(struct Fretless_context* ctxp, int finger,int key,int val);
//Move may balk and only do a partial move
float Fretless_move(struct Fretless_context* ctxp, int finger,float fnote,int polyGroup);
//Free up the finger
void Fretless_up(struct Fretless_context* ctxp, int finger);

//sequence finish.  we can send it now
void Fretless_flush(struct Fretless_context* ctxp);


//Finger up pointer
void Fretless_util_unmapFinger(struct Fretless_context* ctxp, void* ptr);
//Finger move or finger down ptr
int Fretless_util_mapFinger(struct Fretless_context* ctxp, void* ptr);

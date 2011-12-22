//
//  AlephOneTests.m
//  AlephOneTests
//
//  Created by Robert Fielding on 10/14/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#import "AlephOneTests.h"
#import "CoreMIDIRenderer.h"
#import "Fretless.h"

#import <stdio.h>
#import <stdarg.h>

struct Fretless_context* fctx;

@implementation AlephOneTests

- (void)setUp
{
    [super setUp];

}

- (void)tearDown
{
    [super tearDown];
}

-(void)testTheOneThingThatMIDIShouldHaveDoneRight
{
    struct Fretless_context* fctx = Fretless_init(
                                                  CoreMIDIRenderer_midiPutch,
                                                  CoreMIDIRenderer_midiFlush,
                                                  malloc,free,
                                                  CoreMIDIRenderer_midiFail,
                                                  CoreMIDIRenderer_midiPassed,
                                                  printf
                                                  ); 
    
    int finger0 = 0;
    int finger1 = 1;
    int poly0 = 0;
    int poly1 = 1;
    int legato = 1;
    float vol = 1.0;
    /*
    Fretless_boot(fctx);     
    //Put one finger at highest note, and another at the lowest, and bend them in
    //opposite directions.  This is the biggest oversight in standard MIDI that this
    //doesn't work easily.
    float pitch0 = 1.0;
    Fretless_down(fctx, finger0, pitch0, poly0, vol, legato);
    //Fretless_down(fctx, finger1, pitch1, 1, 1.0, legato);
    Fretless_flush(fctx);
    for(pitch0=12; pitch0<100; pitch0+=0.01)
    {
        Fretless_move(fctx, finger0, pitch0, poly0);
        //Fretless_move(fctx, finger1, 127 - pitch0, poly1);
        Fretless_flush(fctx);
        
        [NSThread sleepForTimeInterval:0.01];
    }
    Fretless_up(fctx, finger0);
    //Fretless_up(fctx, finger1);
    Fretless_flush(fctx);
    */
    Fretless_free(fctx);
    
}

@end

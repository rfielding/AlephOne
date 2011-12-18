//
//  AlephOneTests.m
//  AlephOneTests
//
//  Created by Robert Fielding on 10/14/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#import "AlephOneTests.h"
#import "Fretless.h"
#import "CoreMIDIRenderer.h"

#import <stdio.h>
#import <stdarg.h>

@implementation AlephOneTests

- (void)setUp
{
    [super setUp];

}

- (void)tearDown
{
    // Tear-down code here.
    
    [super tearDown];
}

-(void)testTheOneThingThatMIDIShouldHaveDoneRight
{
    struct Fretless_context* fctx = Fretless_init(
                                                         CoreMIDIRenderer_midiPutch,
                                                         CoreMIDIRenderer_midiFlush,
                                                         malloc,
                                                         CoreMIDIRenderer_midiFail,
                                                         CoreMIDIRenderer_midiPassed,
                                                         printf
                                                         );  
    Fretless_boot(fctx);    

    //Put one finger at highest note, and another at the lowest, and bend them in
    //opposite directions.  This is the biggest oversight in standard MIDI that this
    //doesn't work easily.
    Fretless_down(fctx, 0, 0.0, 0, 1.0, 1);
    Fretless_down(fctx, 1, 127.0, 1, 1.0, 1);
    for(float p=0; p<127; p+=0.01)
    {
        Fretless_move(fctx, 0, p, 0);
        Fretless_move(fctx, 1, 127-p, 1);
        Fretless_flush(fctx);
    }
    Fretless_up(fctx, 0);
    Fretless_up(fctx, 1);
}

@end

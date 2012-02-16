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
    
    Fretless_free(fctx);
    
}

@end

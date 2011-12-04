//
//  AlephOneTests.m
//  AlephOneTests
//
//  Created by Robert Fielding on 10/14/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#import "AlephOneTests.h"
#include "Fretless.h"

#import <stdio.h>
#import <stdarg.h>

void midiPutch(char c)
{
    printf("%2x:",((unsigned)c&0xFF));
}

void midiFlush()
{
    printf("\n");
}

int midiFail(const char* msg,...)
{
    va_list argp;
    va_start(argp,msg);
    vfprintf(stderr,msg,argp);
    va_end(argp);
    return 0;
}

void midiPassed()
{
    
}

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


- (void)testDownMoveExpressUp
{    
    int middleFinger = 1;
    int secondFinger = 2;
    float concertA = 33.0;
    float oneCent = 0.01;
    float mezzoForte = 1.0;
    int expr1 = 1;
    int polyGroupNone = 0;
    
    struct Fretless_context* ctxp = Fretless_init(midiPutch,midiFlush,malloc,midiFail,midiPassed,printf);
    Fretless_setMidiHintChannelBase(ctxp,0);
    Fretless_setMidiHintChannelSpan(ctxp,1);
    Fretless_boot(ctxp);
    
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_move(ctxp,middleFinger,concertA+oneCent,polyGroupNone);
    Fretless_flush(ctxp);
    Fretless_express(ctxp,middleFinger,expr1,mezzoForte);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    
    //Check 1 channel interleaving
    Fretless_down(ctxp,secondFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_up(ctxp,secondFinger);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    
}

- (void)testTrill
{    
    int middleFinger = 1;
    int secondFinger = 2;
    float concertA = 33.0;
    float concertB = 35.0;
    float mezzoForte = 1.0;
    int polyGroupNone = 0;
    
    struct Fretless_context* ctxp = Fretless_init(midiPutch,midiFlush,malloc,midiFail,midiPassed,printf);
    Fretless_setMidiHintChannelBase(ctxp,0);
    Fretless_setMidiHintChannelSpan(ctxp,4);
    Fretless_boot(ctxp);
    
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    /*
    Fretless_move(ctxp,middleFinger,concertA+twentyFiveCent);
    Fretless_flush(ctxp);
    Fretless_move(ctxp,middleFinger,concertA+twentyFiveCent+twentyFiveCent);
    Fretless_flush(ctxp);
    Fretless_move(ctxp,middleFinger,concertA+twentyFiveCent+twentyFiveCent+twentyFiveCent);
     */
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    
    
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    Fretless_down(ctxp,middleFinger,concertA+0.1,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    Fretless_down(ctxp,middleFinger,concertA+0.2,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    
    Fretless_down(ctxp,middleFinger,concertA+0.3,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    Fretless_down(ctxp,middleFinger,concertA+0.4,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);

    
    //Interleaving!
    
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_down(ctxp,secondFinger,concertB,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_up(ctxp,secondFinger);
    Fretless_flush(ctxp);
    
}

-(void)testInterleavePolySame
{
    int middleFinger = 1;
    int secondFinger = 2;
    int thirdFinger = 3;
    float concertA = 33.0;
    float concertB = 35.0;
    float concertC = 37.0;
    float mezzoForte = 1.0;
    int polyGroupNone = 0;
    
    struct Fretless_context* ctxp = Fretless_init(midiPutch,midiFlush,malloc,midiFail,midiPassed,printf);
    Fretless_setMidiHintChannelBase(ctxp,0);
    Fretless_setMidiHintChannelSpan(ctxp,4);
    Fretless_setMidiHintChannelBendSemis(ctxp,2);
    Fretless_setMidiHintSupressBends(ctxp, TRUE);
    
    Fretless_boot(ctxp);
    
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_down(ctxp,secondFinger,concertB,polyGroupNone,mezzoForte,0);
    Fretless_down(ctxp,thirdFinger,concertC,polyGroupNone,mezzoForte,0);
    Fretless_up(ctxp,middleFinger);
    Fretless_up(ctxp,secondFinger);
    Fretless_up(ctxp,thirdFinger);
    Fretless_flush(ctxp);
}

-(void)testInterleavePolyDifferent
{
    int middleFinger = 1;
    int secondFinger = 2;
    float concertA = 33.0;
    float concertB = 35.0;
    float mezzoForte = 1.0;
    int polyGroupNone = 0;
    
    struct Fretless_context* ctxp = Fretless_init(midiPutch,midiFlush,malloc,midiFail,midiPassed,printf);
    Fretless_setMidiHintChannelBase(ctxp,0);
    Fretless_setMidiHintChannelSpan(ctxp,4);
    Fretless_setMidiHintChannelBendSemis(ctxp,2);
    Fretless_setMidiHintSupressBends(ctxp, TRUE);
    
    Fretless_boot(ctxp);
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_down(ctxp,secondFinger,concertB,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,secondFinger);
    Fretless_flush(ctxp);
    Fretless_flush(ctxp);
    
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_down(ctxp,secondFinger,concertB,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,secondFinger);
    Fretless_flush(ctxp);
    Fretless_down(ctxp,middleFinger,concertA,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_down(ctxp,secondFinger,concertB,polyGroupNone,mezzoForte,0);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,middleFinger);
    Fretless_flush(ctxp);
    Fretless_up(ctxp,secondFinger);
    Fretless_flush(ctxp);
}
@end

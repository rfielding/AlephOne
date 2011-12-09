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

-(void)testInterleavePolySame
{
}

@end

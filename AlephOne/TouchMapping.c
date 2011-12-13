//
//  TouchMapping.c
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "TouchMapping.h"

#define FINGERMAX 16
#define NOBODY -1
#define NULL 0

static void* utilFingerAlloced[FINGERMAX];

int TouchMapping_mapFinger(void* ptr)
{
    //return an id if we already allocated one for this pointer
    for(int f=0; f<FINGERMAX; f++)
    {
        if(utilFingerAlloced[f] == ptr)
        {
            return f;
        }
    }
    //otherwise, map into a location and return that
    for(int f=0; f<FINGERMAX; f++)
    {
        if(utilFingerAlloced[f] == NULL)
        {
            utilFingerAlloced[f] = ptr;
            return f;
        }
    }
    //ctxp->fail("Fretless_util_mapFinger ran out of slots\n");
    return NOBODY;
}

void TouchMapping_unmapFinger(void* ptr)
{
    for(int f=0; f<FINGERMAX; f++)
    {
        if(utilFingerAlloced[f] == ptr)
        {
            utilFingerAlloced[f] = NULL;
            return;
        }
    }    
    //ctxp->fail("Fretless_util_unmapFinger tried to unmap an unmapped pointer\n");
}

//Provide a second mapping for the finger, which is useful for voice doubling
int TouchMapping_mapFinger2(void* touch)
{
    return TouchMapping_mapFinger((void*)((int)touch ^ 0xFFFFFFFF));
}

void TouchMapping_unmapFinger2(void* touch)
{
    TouchMapping_unmapFinger((void*)((int)touch ^ 0xFFFFFFFF));
}

int TouchMapping_finger2FromFinger1(int finger)
{
    void* finger1Touch = utilFingerAlloced[finger];
    void* finger2 = (void*) ((int)finger1Touch ^ 0xFFFFFFFF);
    return TouchMapping_mapFinger(finger2);
}





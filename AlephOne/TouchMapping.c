//
//  TouchMapping.c
//  AlephOne
//
//  Created by Robert Fielding on 12/1/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "TouchMapping.h"
#include "FretlessCommon.h"

#include <stdio.h>

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
            //printf("TouchMapping_mapFinger %d -> %d\n",ptr, f);
            if((int)ptr == -1)
            {
                printf("unmapped something that doesn't look like a pointer: -1\n");
            }
            utilFingerAlloced[f] = ptr;
            return f;
        }
    }
    printf("TouchMapping_mapFinger ran out of slots!\n");
    return NOBODY;
}

void TouchMapping_unmapFinger(void* ptr)
{
    for(int f=0; f<FINGERMAX; f++)
    {
        if(utilFingerAlloced[f] == ptr)
        {
            //printf("TouchMapping_unmapFinger %d !-> %d\n",ptr, f);
            utilFingerAlloced[f] = NULL;
            return;
        }
    }    
    printf("TouchMapping_unmapFinger tried to unmap an unmapped pointer\n");
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





//
//  WidgetTree.c
//  AlephOne
//
//  Created by Robert Fielding on 12/21/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "WidgetTree.h"
#include "FretlessCommon.h"

#define MAXWIDGETS 256
struct WidgetTree_rect rectangles[MAXWIDGETS];
int rectanglesCount = 0;
int (*fail)(const char*,...);
int (*logger)(const char*,...);


void WidgetTree_init(    
    int (*failArg)(const char*,...),
    int (*loggerArg)(const char*,...)
)
{
    rectanglesCount = 0;
    fail = failArg;
    logger = loggerArg;
}

struct WidgetTree_rect* WidgetTree_hitTest(float x,float y)
{
    if(rectanglesCount<1)fail("Can't hit test.  Root widget must be added first.\n");
    int idx = rectanglesCount;
    while(idx >= 0)
    {
        if(
           rectangles[idx].x1 <= x &&
           rectangles[idx].y1 <= y &&
           rectangles[idx].x2 >= x &&
           rectangles[idx].y2 >= y && 
           rectangles[idx].down &&
           rectangles[idx].isActive)
            return &rectangles[idx];
        idx--;
    }
    //If out of bounds, it hit tests against the root
    return &rectangles[0];
}

int WidgetTree_count()
{
    return rectanglesCount;
}

struct WidgetTree_rect* WidgetTree_get(int order)
{
    if(order < 0 || rectanglesCount <= order)
    {
        fail("out of bounds widget tree index: %d\n", order);
    }
    return &rectangles[order];
}



struct WidgetTree_rect* WidgetTree_add(float x1, float y1, float x2, float y2)
{
    if(rectanglesCount + 1 >= MAXWIDGETS)
    {
        fail("added too many widgets!");
        return NULL;        
    }
    rectangles[rectanglesCount].x1 = x1;
    rectangles[rectanglesCount].y1 = y1;
    rectangles[rectanglesCount].x2 = x2;
    rectangles[rectanglesCount].y2 = y2;
    rectangles[rectanglesCount].tick = NULL;
    rectangles[rectanglesCount].render = NULL;
    rectangles[rectanglesCount].up = NULL;
    rectangles[rectanglesCount].down = NULL;
    rectangles[rectanglesCount].flush = NULL;
    rectangles[rectanglesCount].ctx = NULL;
    rectangles[rectanglesCount].isActive = 1;
    rectanglesCount++;
    return &rectangles[rectanglesCount-1];
}
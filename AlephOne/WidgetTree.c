//
//  WidgetTree.c
//  AlephOne
//
//  Created by Robert Fielding on 12/21/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "WidgetTree.h"

#define NULL ((void*)0)
#define MAXWIDGETS 256
struct WidgetTree_rect rectangles[MAXWIDGETS];
int rectanglesCount = 0;

int WidgetTree_hitTest(float x,float y)
{
    int idx = rectanglesCount;
    while(idx >= 0)
    {
        if(
           rectangles[idx].x1 <= x &&
           rectangles[idx].y1 <= y &&
           rectangles[idx].x2 >= x &&
           rectangles[idx].y2 >= y && 
           rectangles[idx].down)
            return rectangles[idx].identifier;
        idx--;
    }
    //If out of bounds, it hit tests against the root
    return ROOTWIDGET;
}

int WidgetTree_count()
{
    return rectanglesCount;
}

struct WidgetTree_rect* WidgetTree_get(int identifier)
{
    int idx = 0;
    while(idx < rectanglesCount)
    {
        if(idx >= MAXWIDGETS)return NULL;
        
        if(rectangles[idx].identifier == identifier)
            return &rectangles[idx];
        idx++;
    }
    return NULL;    
}

void WidgetTree_clear()
{
    rectanglesCount = 0;
}

struct WidgetTree_rect* WidgetTree_add(int identifier, float x1, float y1, float x2, float y2)
{
    if(rectanglesCount + 1 >= MAXWIDGETS)return NULL;
    rectangles[rectanglesCount].x1 = x1;
    rectangles[rectanglesCount].y1 = y1;
    rectangles[rectanglesCount].x2 = x2;
    rectangles[rectanglesCount].y2 = y2;
    rectangles[rectanglesCount].identifier = identifier;
    rectangles[rectanglesCount].tick = NULL;
    rectangles[rectanglesCount].render = NULL;
    rectangles[rectanglesCount].up = NULL;
    rectangles[rectanglesCount].down = NULL;
    rectangles[rectanglesCount].flush = NULL;
    rectangles[rectanglesCount].ctx = NULL;
    rectanglesCount++;
    return &rectangles[rectanglesCount-1];
}
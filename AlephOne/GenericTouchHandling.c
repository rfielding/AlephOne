//
//  GenericTouchHandling.c
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "GenericTouchHandling.h"
#include "TouchMapping.h"
#include "WidgetTree.h"

#include "SurfaceTouchHandling.h"
#include "FretlessCommon.h"

struct Fretless_context;
struct PitchHandler_context;

static struct Fretless_context* fretlessp = NULL;
static struct PitchHandler_context* phctx = NULL;
static int (*fail)(const char*,...);
static int (*logger)(const char*,...);
static struct WidgetTree_rect* currentWidget[FINGERMAX];

void GenericTouchHandling_touchesFlush()
{
    SurfaceTouchHandling_touchesFlush(NULL);
}

void GenericTouchHandling_touchesInit(
    struct PitchHandler_context* phctxArg, 
    struct Fretless_context* fctxArg,
    int (*failArg)(const char*,...),
    int (*loggerArg)(const char*,...)
)
{
    phctx = phctxArg;
    fretlessp = fctxArg;
    fail = failArg;
    logger = loggerArg;
        
    SurfaceTouchHandling_touchesInit(phctxArg,fctxArg,failArg,loggerArg);
    
    for(int f=0; f<FINGERMAX; f++)
    {
        currentWidget[f] = NULL;
    }
}

void GenericTouchHandling_touchesUp(void* touch)
{
    int finger  = TouchMapping_mapFinger(touch);
    if(finger<0)fail("impossible state: finger is invalid: %d",finger);    
    struct WidgetTree_rect* itemP = currentWidget[finger];
    if(!itemP)fail("impossible state touchesUp: currentWidget for finger %d is not set.",finger);
    if(itemP && itemP->up)
    {
        itemP->up(itemP->ctx,finger,touch);        
    }
    //We must unmap it unconditionally
    TouchMapping_unmapFinger(touch);        
}


void GenericTouchHandling_touchesDown(void* touch,int isMoving,float x,float y, float velocity, float area)
{
    int finger  = TouchMapping_mapFinger(touch);
    if(finger<0)fail("impossible state: finger is invalid: %d",finger);    
    
    //We can only perform hit tests on finger down
    if(!isMoving)
    {
        currentWidget[finger] = WidgetTree_hitTest(x,y);
    }
    
    struct WidgetTree_rect* itemP = currentWidget[finger];
    if(!itemP)fail("impossible state touchesDown: currentWidget for finger %d is not set.",finger);    
    if(itemP && itemP->down)
    {
        itemP->down(itemP->ctx,finger,touch,isMoving,x,y,velocity,area);                    
    }        
}

void GenericTouchHandling_tick()
{
    SurfaceTouchHandling_tick(NULL);        
}


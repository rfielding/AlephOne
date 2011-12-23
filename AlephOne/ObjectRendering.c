//
//  ObjectRendering.c
//  AlephOne
//
//  Created by Robert Fielding on 12/18/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//
/**
   This place is a bit of a mosh pit of dependencies, because it wires together a lot of
   functions.
 */

#include "VertexObjectBuilder.h"
#include "ObjectRendering.h"
#include "Fretless.h"
#include "PitchHandler.h"
#include "WidgetTree.h"

#include "SurfaceTouchHandling.h"
#include "SurfaceDrawHandling.h"

#define NULL ((void*)0)
#include <math.h>
#include <stdlib.h>

#include "WidgetConstants.h"
#include "ChannelOccupancyControl.h"
#include "SliderControl.h"


static void* ObjectRendering_imageContext;
static void (*ObjectRendering_imageRender)(void*,char*,unsigned int*,float*,float*);
static void (*ObjectRendering_stringRender)(void*,char*,unsigned int*,float*,float*);
static void (*ObjectRendering_drawVO)();

static int triangles;
static int trianglestrip;
static int linestrip;

static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;
static struct Fretless_context* fctx;

static char* requiredTexture[] = {
    "tutorial",
    "ashmedi",
    "stars",
    "channelcycling"
};

#define IMAGECOUNT 4



static unsigned int textures[256];
static float textureWidth[256];
static float textureHeight[256];
static float lightPosition[3];


void WidgetsAssemble();

void ObjectRendering_updateLightOrientation(float x,float y, float z)
{
    lightPosition[0] = x;
    lightPosition[1] = y;
    lightPosition[2] = z;
}

void ObjectRendering_init(
                           struct VertexObjectBuilder* voCtxDynamicArg,
                           struct PitchHandler_context* phctxArg,
                           struct Fretless_context* fctxArg,
                           int trianglesArg,
                           int trianglestripArg,
                           int linestripArg,
                           void* ObjectRendering_imageContextArg,
                           void (*ObjectRendering_imageRenderArg)(void*,char*,unsigned int*,float*,float*),
                           void (*ObjectRendering_stringRenderArg)(void*,char*,unsigned int*,float*,float*),
                           void (*ObjectRendering_drawVOArg)()
                           )
{
    voCtxDynamic = voCtxDynamicArg;
    phctx = phctxArg;
    fctx = fctxArg;
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    linestrip = linestripArg;
    ObjectRendering_imageContext = ObjectRendering_imageContextArg;
    ObjectRendering_imageRender = ObjectRendering_imageRenderArg;
    ObjectRendering_stringRender = ObjectRendering_stringRenderArg;
    ObjectRendering_drawVO       = ObjectRendering_drawVOArg;
  
    SurfaceDraw_init(voCtxDynamicArg,phctxArg,trianglesArg,trianglestripArg);
    SliderControl_init(voCtxDynamicArg,phctxArg,trianglesArg,trianglestripArg);
    ChannelOccupancyControl_touchesInit(triangles,trianglestrip,linestrip,voCtxDynamicArg, fctxArg);
    WidgetsAssemble();
}

void ObjectRendering_loadImages()
{
    for(int i=0; i < IMAGECOUNT; i++)
    {
        ObjectRendering_imageRender(
            ObjectRendering_imageContext,
            requiredTexture[i],
            &textures[i],
            &textureWidth[i],
            &textureHeight[i]
        );
    }
}

int ObjectRendering_getTexture(int idx)
{
    return textures[idx];
}


void GenericRendering_draw()
{
    VertexObjectBuilder_reset(voCtxDynamic);    
    
    //Opposite order of hit testing to draw them in layers
    for(int item=0; item<WidgetTree_count(); item++)
    {
        struct WidgetTree_rect* itemP = WidgetTree_get(item);
        if(itemP->render)
        {
            itemP->render(itemP->ctx);            
        }
    }
    ObjectRendering_drawVO(voCtxDynamic);    
}

/**
 * A strange place for this function in some ways, but it doesn't create anything terrible like
 * OS dependencies, etc.
 */
void WidgetsAssemble()
{
    struct WidgetTree_rect* itemP = NULL;
    int widget=0;
    
    itemP = WidgetTree_add(widget,0,0,1,1);
    itemP->render = SurfaceDraw_render;
    itemP->up = SurfaceTouchHandling_touchesUp;
    itemP->down = SurfaceTouchHandling_touchesDown;
    itemP->tick = SurfaceTouchHandling_tick;
    itemP->flush = SurfaceTouchHandling_touchesFlush;
    
    widget++;
    itemP = WidgetTree_add(widget, 0.7 - 0.2, 0.7 - 0.2, 0.7 + 0.2, 0.7 + 0.2);    
    itemP->render = ChannelOccupancyControl_render;    
    
    widget++;
    itemP = CreateSlider(widget, 0,0.9, 1,1, NULL, NULL, Slider_render);
}
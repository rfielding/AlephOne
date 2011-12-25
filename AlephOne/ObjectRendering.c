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
#include "Fret.h"
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
static void (*ObjectRendering_imageRender)(void*,char*,unsigned int*,float*,float*,int);
static void (*ObjectRendering_stringRender)(void*,char*,unsigned int*,float*,float*,int);
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


#define TEXTURECOUNT 256
static unsigned int textures[TEXTURECOUNT];
static float textureWidth[TEXTURECOUNT];
static float textureHeight[TEXTURECOUNT];
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
                           void (*ObjectRendering_imageRenderArg)(void*,char*,unsigned int*,float*,float*,int),
                           void (*ObjectRendering_stringRenderArg)(void*,char*,unsigned int*,float*,float*,int),
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

//Pass a funtion pointer to this for anything that needs to update a string
//The impl might need to maintain more state to be efficient.
void reRenderString(unsigned int texture,char* val,int* w, int* h)
{
    //These need to be re-rendered into the same slot
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 val,
                                 &textures[texture],
                                 &textureWidth[texture],
                                 &textureHeight[texture],
                                 0
                                 );    
    *w = textureWidth[texture];
    *h = textureHeight[texture];
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
            &textureHeight[i],
            0
        );
    }
    //These need to be re-rendered into the same slot
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 "Channel Cycling",
                                 &textures[PIC_CHANNELCYCLINGTEXT],
                                 &textureWidth[PIC_CHANNELCYCLINGTEXT],
                                 &textureHeight[PIC_CHANNELCYCLINGTEXT],
                                 0
                                 );
    //These need to be re-rendered into the same slot
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 "Center",
                                 &textures[PIC_BASENOTETEXT],
                                 &textureWidth[PIC_BASENOTETEXT],
                                 &textureHeight[PIC_BASENOTETEXT],
                                 0
                                 );    
    //These need to be re-rendered into the same slot
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 "Scale",
                                 &textures[PIC_SCALETEXT],
                                 &textureWidth[PIC_SCALETEXT],
                                 &textureHeight[PIC_SCALETEXT],
                                 0
                                 );       
    //These need to be re-rendered into the same slot
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 "Width",
                                 &textures[PIC_WIDTHTEXT],
                                 &textureWidth[PIC_WIDTHTEXT],
                                 &textureHeight[PIC_WIDTHTEXT],
                                 0
                                 );       
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

void NoteDiff_set(void* ctx, float val)
{
    PitchHandler_setNoteDiff(phctx, 24-1+24*val);
}

float NoteDiff_get(void* ctx)
{
    return (PitchHandler_getNoteDiff(phctx)-23)/24.0;
}

void Cols_set(void* ctx, float val)
{
    PitchHandler_setColCount(phctx, 5 + val*7);
}

float Cols_get(void* ctx)
{
    return (PitchHandler_getColCount(phctx)-5)/7;
}

void Intonation_set(void* ctx, float val)
{
    struct Fret_context* frctx = PitchHandler_frets(phctx);
    Fret_clearFrets(frctx);
    
    float baseNote = 2.0;
    if(val > 0.9)
    {
        Fret_placeFret(frctx, baseNote + 12*0,  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(9.0/8),  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(6.0/5),  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(4.0/3),  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(3.0/2),  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(4.0/5 * 2), 2);
        Fret_placeFret(frctx, baseNote + 12*log2f(8.0/9 * 2), 3);
    }
    else
    {
        if(val >= 0)
        {
            Fret_placeFret(frctx,baseNote +  0.0,3);
            Fret_placeFret(frctx,baseNote +  3.0,3);            
            Fret_placeFret(frctx,baseNote +  5.0,3);        
            Fret_placeFret(frctx,baseNote +  7.0,3);
            Fret_placeFret(frctx,baseNote + 10.0,3);
        }
        if(val > 0.35)
        {
            Fret_placeFret(frctx,baseNote +  2.0,3);
            Fret_placeFret(frctx,baseNote +  6.0,2);
            Fret_placeFret(frctx,baseNote +  9.0,3);        
        }
        if(val > 0.5)
        {
            Fret_placeFret(frctx,baseNote +  1.0,2);
            Fret_placeFret(frctx,baseNote +  4.0,2);
            Fret_placeFret(frctx,baseNote +  8.0,2);
            Fret_placeFret(frctx,baseNote + 11.0,2);                
        }
        if(val > 0.65 || (val < 0.30 && val > 0.25))
        {
            Fret_placeFret(frctx,baseNote + 1.5,1);
            Fret_placeFret(frctx,baseNote + 8.5,1);                
        }
        if(val > 0.65)
        {
            Fret_placeFret(frctx,baseNote + 6.5,1);        
        }
        if(val > 0.8)
        {
            Fret_placeFret(frctx,baseNote +  0.5,1);
            Fret_placeFret(frctx,baseNote +  2.5,1);
            Fret_placeFret(frctx,baseNote +  3.5,1);                                    
            Fret_placeFret(frctx,baseNote +  4.5,1);                                    
            Fret_placeFret(frctx,baseNote +  5.5,1);                                    
            Fret_placeFret(frctx,baseNote +  7.5,1);                                    
            Fret_placeFret(frctx,baseNote +  9.5,1);                                    
            Fret_placeFret(frctx,baseNote + 10.5,1);                                    
            Fret_placeFret(frctx,baseNote + 11.5,1);                                                                   
        }        
    }
}



/**
 * A strange place for this function in some ways, but it doesn't create anything terrible like
 * OS dependencies, etc.
 */
void WidgetsAssemble()
{
    struct WidgetTree_rect* itemP = NULL;
    int widget=0;
    
    itemP = SurfaceDraw_create();    
    widget++;

    float cx = 0.8;
    float cy = 0.2;
    itemP = WidgetTree_add(widget, cx - 0.2, cy - 0.2, cx + 0.2, cy + 0.2);    
    itemP->render = ChannelOccupancyControl_render;        
    widget++;
    
    itemP = CreateSlider(widget, PIC_BASENOTETEXT,0,0.9, 0.33,1, NoteDiff_set, NoteDiff_get, Slider_render);
    widget++;    
    
    itemP = CreateSlider(widget, PIC_SCALETEXT,0.332,0.9, 0.66,1, Intonation_set, NULL, Slider_render);
    widget++;      
    
    itemP = CreateSlider(widget, PIC_WIDTHTEXT,0.662,0.9, 1,1, Cols_set, Cols_get, Slider_render);
    widget++;      
}
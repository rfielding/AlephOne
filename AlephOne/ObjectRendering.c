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

//#define NULL ((void*)0)
#define FALSE 0
#define TRUE 1
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "WidgetConstants.h"
#include "ChannelOccupancyControl.h"
#include "SliderControl.h"
#include "ButtonControl.h"


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

//All the references to controls
struct Slider_data* baseSlider;
struct Slider_data* intonationSlider;
struct Slider_data* widthSlider;
struct Slider_data* heightSlider;

struct Slider_data* rootNoteSlider;

static float baseNote = 2.0;

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
    SliderControl_init(voCtxDynamicArg,phctxArg,trianglesArg,trianglestripArg,linestripArg);
    ButtonControl_init(voCtxDynamicArg,phctxArg,trianglesArg,trianglestripArg);
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

void renderLabel(char* label, int texture)
{
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 label,
                                 &textures[texture],
                                 &textureWidth[texture],
                                 &textureHeight[texture],
                                 0
                                 );    
}

//This is called when we have set up the OpenGL context already
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
    //Loading up strings because we know that OpenGL context is now valid.
    //This may move to support re-rendering of strings
    renderLabel("Channel Cycling", PIC_CHANNELCYCLINGTEXT);
    renderLabel("Center",PIC_BASENOTETEXT);
    renderLabel("Scale",PIC_SCALETEXT);
    renderLabel("Width",PIC_WIDTHTEXT);
    renderLabel("Page",PIC_PAGE1TEXT);
    renderLabel("Circle Of Fifths", PIC_ROOTNOTETEXT);
    renderLabel("Height", PIC_HEIGHTTEXT);
}

int ObjectRendering_getTexture(int idx)
{
    return textures[idx];
}

//Rendering is just drawing all active and renderable items in order
void GenericRendering_draw()
{
    VertexObjectBuilder_reset(voCtxDynamic);    
    
    for(int item=0; item<WidgetTree_count(); item++)
    {
        struct WidgetTree_rect* itemP = WidgetTree_get(item);
        if(itemP->render && itemP->isActive)
        {
            itemP->render(itemP->ctx);            
        }
    }
    ObjectRendering_drawVO(voCtxDynamic);    
}

//Control paging is simply hiding and showing controls
void Page_set(void* ctx, int val)
{
    baseSlider->rect->isActive = FALSE;
    intonationSlider->rect->isActive = FALSE;
    widthSlider->rect->isActive = FALSE;
    heightSlider->rect->isActive = FALSE;
    rootNoteSlider->rect->isActive = FALSE;
    
    switch(val)
    {
        case 0:
            baseSlider->rect->isActive = TRUE;
            widthSlider->rect->isActive = TRUE;
            heightSlider->rect->isActive = TRUE;
            break;
        case 1:
            intonationSlider->rect->isActive = TRUE;
            rootNoteSlider->rect->isActive = TRUE;
            break;
    }
}

int Page_get(void* ctx)
{
    return 0;
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

void Rows_set(void* ctx, float val)
{
    PitchHandler_setRowCount(phctx, 2 + val*7);
}

float Rows_get(void* ctx)
{
    return (PitchHandler_getRowCount(phctx)-2)/7;
}

float Intonation_get(void* ctx)
{
    return ((struct Slider_data*)ctx)->val;
}

void Intonation_do(float val)
{
    struct Fret_context* frctx = PitchHandler_frets(phctx);
    Fret_clearFrets(frctx);
    
    if(val > 0.9)
    {
        Fret_placeFret(frctx, baseNote + 12*0,  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(6.0/5),  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(4.0/3),  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(3.0/2),  3);
        Fret_placeFret(frctx, baseNote + 12*log2f(4.0/5 * 2), 2);
        Fret_placeFret(frctx, baseNote + 12*log2f(8.0/9 * 2), 3);
        
        if(val > 0.95)
        {
            Fret_placeFret(frctx, baseNote + 12*log2f(13.0/12),  1);                        
        }
        else
        {
            Fret_placeFret(frctx, baseNote + 12*log2f(9.0/8),  3);            
        }
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

void Intonation_set(void* ctx, float val)
{
    Intonation_do(val);
}

void RootNote_set(void* ctx, float val)
{
    baseNote = (int)(7*((int)(12*val-4+12)) % 12);
    //printf("%d\n",(int)baseNote);
    Intonation_do(intonationSlider->val);
}

float RootNote_get(void* ctx)
{
    return ((struct Slider_data*)ctx)->val;
}


/**
 *  This assembles all of the controls, and invokes the callbacks, usually locally defined.
 */
void WidgetsAssemble()
{
    struct WidgetTree_rect* itemP = NULL;
    
    itemP = SurfaceDraw_create();    

    float cx = 0.8;
    float cy = 0.2;
    
    //Creating a raw control given just a rendering function
    ChannelOccupancyControl_create(cx - 0.2, cy - 0.2, cx + 0.2, cy + 0.2);
    
    float panelBottom = 0.9;
    float panelTop = 1.0;
    //This button cycles through pages of controls
    CreateButton(PIC_PAGE1TEXT,0.0,0.9, 0.11,1, Page_set, Page_get, Button_render, 3);
    
    //Page 1
    baseSlider = CreateSlider(PIC_BASENOTETEXT,0.12,panelBottom, 0.33,panelTop, NoteDiff_set, NoteDiff_get);    
    widthSlider = CreateSlider(PIC_WIDTHTEXT,0.332,panelBottom, 0.66,panelTop, Cols_set, Cols_get);
    heightSlider = CreateSlider(PIC_HEIGHTTEXT,0.662,panelBottom, 1,panelTop, Rows_set, Rows_get);
    
    //Page 2
    intonationSlider = CreateSlider(PIC_SCALETEXT,0.12,panelBottom, 0.5,panelTop, Intonation_set, Intonation_get);
    rootNoteSlider = CreateSlider(PIC_ROOTNOTETEXT,0.502,panelBottom, 1,panelTop, RootNote_set, RootNote_get);

    //Set us to page 0 to start
    Page_set(NULL, 0);
}
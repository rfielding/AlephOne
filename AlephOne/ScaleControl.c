//
//  ScaleControl.c
//  AlephOne
//
//  Created by Robert Fielding on 1/5/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//

#include "ScaleControl.h"
#include "PitchHandler.h"
#include "VertexObjectBuilder.h"
#include <math.h>
#include "WidgetTree.h"
#include "Fretless.h"
#include "Fret.h"

#include "WidgetConstants.h"
#include <stdlib.h>
#include <stdio.h>

static int triangles;
static int trianglestrip;
static int linestrip;
static int lines;
static struct VertexObjectBuilder* voCtxDynamic;
static struct Fretless_context* fctx;
#define RINGS 7
int etPicked[106][RINGS];
int ets[] = {7,12,19,24,31,53,106};
int ring=0;
int fret=0;
#define ETRADIUS 0.065

void ScaleControl_clear(void* ctx)
{
    for(int r=0; r<RINGS; r++)
    {
        for(int f=0; f<106; f++)
        {
            etPicked[f][r] = 0;
        }
        etPicked[0][r] = 1;
    }    
}

void ScaleControl_touchesInit(
        int trianglesArg,
        int trianglestripArg,
        int linestripArg,
        int linesArg,
        struct VertexObjectBuilder* voCtxDynamicArg, 
        struct Fretless_context* fctxArg
                                         )
{
    fctx = fctxArg;
    voCtxDynamic = voCtxDynamicArg;
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    linestrip = linestripArg;
    lines = linesArg;
    

    ScaleControl_clear(NULL);
}

void ScaleControl_render(void* ctx)
{
    struct WidgetTree_rect* panel = ((struct ScaleControl_data*)ctx)->rect;
    float cx = (panel->x1 + panel->x2)/2;
    float cy = (panel->y1 + panel->y2)/2;
    float diameter = (panel->x2 - panel->x1);
    float x1 = panel->x1;
    float y1 = panel->y1;
    float x2 = panel->x2;
    float y2 = panel->y2;
    VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x1, y1, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x1, y2, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x2, y1, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x2, y2, 0, 0,0,0,200); 

    x1 = panel->x1;
    y1 = panel->y1;
    x2 = panel->x2/3;
    y2 = panel->y2/8;
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,PIC_SCALECONTROLTEXT);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x1, y1, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x1, y2, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x2, y1, 0, 1,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x2, y2, 0, 1,1);    
    
    
    float r = 0;
    float r2 = r + diameter*ETRADIUS;
    for(int e=0; e<RINGS; e++)
    {
        int et = ets[e];
        VertexObjectBuilder_startColoredObject(voCtxDynamic,lines);    
        for(int note=0; note<=et; note++)
        {
            float a = note/(1.0*et) * 2*M_PI;
            float cosA = cosf(a);
            float sinA = sinf(a);
            float x1 = cx+r*sinA;
            float y1 = cy+r*cosA;
            float x2 = cx+r2*sinA;
            float y2 = cy+r2*cosA;
            int r = 0;
            int g = 100;
            int b = 100;
            if(etPicked[note][e])
            {
                r=0;
                g=255;
                b=255;
            }
            if(note==fret && e==ring)
            {
                r=255;
                g=255;
                b=255;
            }
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,x1,y1,0, r, g, b,255);                            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,x2,y2,0, r, g, b,255);                            
        }
        r = r2;
        r2 = r + diameter*ETRADIUS;
    }
}

void ScaleControl_down(void* ctx,int finger,void* touch,int isMoving,float x,float y, float velocity, float area)
{
    struct ScaleControl_data* scaleControl = (struct ScaleControl_data*)ctx;
    if(scaleControl)
    {
        struct WidgetTree_rect* panel = scaleControl->rect;
        float cx = (panel->x1 + panel->x2)/2;
        float cy = (panel->y1 + panel->y2)/2;
        float diameter = (panel->x2 - panel->x1);
        float dist = sqrtf((x-cx)*(x-cx) + (y-cy)*(y-cy));
        float r = diameter*ETRADIUS;
        int iring = dist / r;
        if(iring < RINGS)
        {
            float angle = fmod((0.5/ets[iring] + 0.25 - atan2f((y-cy),(x-cx))/(M_PI*2))+1.0,1.0);
            
            ring = iring;
            fret = (int) ( angle * ets[ring] - 0.5/ets[ring] );            
        }
    }
}



void ScaleControl_toggle(void* ctx)
{
    etPicked[fret][ring] = !etPicked[fret][ring];
}

//Okay, a little wierd to pass in fretContext just in this one case...
void ScaleControl_commit(void* ctx)
{
    struct Fret_context* fretContext = (struct Fret_context*)ctx;
    Fret_clearFrets(fretContext);
    Fret_placeFret(fretContext, 0, 3);
    for(int r=0; r<RINGS; r++)
    {
        for(int f=0; f<106; f++)
        {
            if(etPicked[f][r])
            {
                float pitch = 12.0 * f / (1.0*ets[r]);
                Fret_placeFret(fretContext, pitch,2);
            }
        }
    }
}

struct ScaleControl_data* ScaleControl_create(float x1,float y1, float x2,float y2)
{
    struct ScaleControl_data* data = 
        (struct ScaleControl_data*)malloc(sizeof(struct ScaleControl_data));
    
    data->rect = WidgetTree_add(x1,y1,x2,y2);    
    data->rect->render = ScaleControl_render;   
    data->rect->ctx = data;
    data->rect->down = ScaleControl_down;
    return data;
}

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
#include "GraphicsCommon.h"

static struct VertexObjectBuilder* voCtxDynamic;
static struct Fretless_context* fctx;
static struct Fret_context* fretCtx;

static char stringRenderBuffer[1024];
void (*reRenderStringFn)(char*,unsigned int);

#define SPRATIOS 13
#define RINGS 7
#define MAXFRETS 665
#define SCALES 8
int etPicked[MAXFRETS][RINGS][SCALES];
int ets[] = {7,12,19,24,31,53,665};
int ring=0;
int fret=0;
int currentScale=0;
int currentBase=0;
float spRatios[SPRATIOS];

#define ETRADIUS (0.058)




void ScaleControl_clear(void* ctx)
{
    for(int r=0; r<RINGS; r++)
    {
        for(int f=0; f<MAXFRETS; f++)
        {
            etPicked[f][r][currentScale] = 0;
        }
        etPicked[0][r][currentScale] = 4;
    }    
    spRatios[0] = log2f(4.0/3);
    spRatios[1] = log2f(5.0/4);
    spRatios[2] = log2f(6.0/5);
    spRatios[3] = log2f(7.0/6);
    spRatios[4] = log2f(8.0/7);
    spRatios[5] = log2f(9.0/8);
    spRatios[6] = log2f(10.0/9);
    spRatios[7] = log2f(11.0/10);
    spRatios[8] = log2f(12.0/11);
    spRatios[9] = log2f(13.0/12);
    spRatios[10] = log2f(14.0/13);
    spRatios[11] = log2f(15.0/14);
    spRatios[12] = log2f(16.0/15);
}



void ScaleControl_init(
        struct VertexObjectBuilder* voCtxDynamicArg, 
        struct Fretless_context* fctxArg,
        struct Fret_context* fretCtxArg,
        void (*reRenderStringArg)(char*,unsigned int)
    )
{
    fctx = fctxArg;
    voCtxDynamic = voCtxDynamicArg;
    fretCtx = fretCtxArg;
    reRenderStringFn = reRenderStringArg;
    ScaleControl_clear(NULL);
    ScaleControl_defaults(NULL);
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
    VertexObjectBuilder_startColoredObject(voCtxDynamic,GRAPHICS_TRIANGLE_STRIP);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x1, y1, 0, 0,0,0,220);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x1, y2, 0, 0,0,0,220);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x2, y1, 0, 0,0,0,220);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, x2, y2, 0, 0,0,0,220); 

    x1 = panel->x1;
    y1 = panel->y1;
    x2 = panel->x2/3;
    y2 = panel->y2/8;
    
    float tau = fret/(1.0*ets[ring]);
    float cents = (1200*tau);
    sprintf(stringRenderBuffer,"%.2fcents %dfret %det",cents,fret,ets[ring]);
    reRenderStringFn(stringRenderBuffer, PIC_SCALECONTROLTEXT);
    
    float r = 0;
    float r2 = r + diameter*ETRADIUS;
    
    //Diatonic markers hardcoded in
    VertexObjectBuilder_startColoredObject(voCtxDynamic,GRAPHICS_LINES);
    for(int f=0; f<12; f++)
    {
        float a = f * 2*M_PI/12.0;
        float cosA = cosf(a);
        float sinA = sinf(a);
        float x1 = cx+r*sinA;
        float y1 = cy+r*cosA;
        float x2 = cx+RINGS*r2*sinA;
        float y2 = cy+RINGS*r2*cosA;
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x1,y1,0,100,100,100,200);                            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x2,y2,0,100,100,100,200);                                    
    }
    
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,GRAPHICS_TRIANGLE_STRIP,PIC_SCALECONTROLTEXT);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x1, y1, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x1, y2, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x2, y1, 0, 1,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x2, y2, 0, 1,1);    
        
    for(int e=0; e<RINGS; e++)
    {
        int et = ets[e];
        VertexObjectBuilder_startColoredObject(voCtxDynamic,GRAPHICS_LINES);    
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
            if(etPicked[note][e][currentScale])
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
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,x1,y1,0, r, g, b,200);                            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,x2,y2,0, r, g, b,200);                            
            if(etPicked[note][e][currentScale])
            {
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r2*sinA,cy+r2*cosA,0, 200,100,200,200);                            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+0.5*diameter*sinA,cy+0.5*diameter*cosA,0, 200,100,200,200);                                            
            }
        }
        r = r2;
        r2 = r + diameter*ETRADIUS;
    }
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx,cy+r2,0, 255, 255, 255,180);                            
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx,cy,0, 255, 255, 255,180);                            
    
    //Show harmonics versus the currently cursor-ed note at angle tau
    //Diatonic markers hardcoded in
    VertexObjectBuilder_startColoredObject(voCtxDynamic,GRAPHICS_LINES);
    for(int i=0; i<SPRATIOS; i++)
    {
        float a = (tau+spRatios[i]) * 2*M_PI;
        float na = (tau-spRatios[i]) * 2*M_PI;
        float cosA = cosf(a);
        float sinA = sinf(a);
        float cosNA = cosf(na);
        float sinNA = sinf(na);
        float x1 = cx;//cx+r*sinA;
        float y1 = cy;//cy+r*cosA;
        float x2 = cx+r2*sinA;
        float y2 = cy+r2*cosA;
        float x3 = cx+r2*sinNA;
        float y3 = cy+r2*cosNA;
        int alpha;
        float r=255;
        float g=255;
        float b=255;
        if(i<1)
        {
            g=0;
            r=0;
            alpha=200;
        }
        else
        {
            if(i<3)
            {
                b=0;
                r=0;
                alpha=175;
            }
            else
            {
                if(i<5)
                {
                    b=50;
                    r=150;
                    g=150;
                    alpha=100;
                }
                else
                {
                    if(i<7)
                    {
                        r=100;
                        b=100;
                        g=100;
                        alpha=100;
                    }
                    else
                    {
                        if(i<10)
                        {
                            b=0;
                            g=0;
                            alpha=100;                            
                        }
                        else
                        {
                            r=100;
                            b=100;
                            g=100;
                            alpha=100;                            
                        }
                    }
                }
            }
        }
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x1,y1,0,r,g,b,0);                            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x2,y2,0,r,g,b,alpha);                                    
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x1,y1,0,r,g,b,0);                            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x3,y3,0,r,g,b,alpha);                                    
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





void ScaleControl_commit(void* ctx)
{
    //printf("clearing frets for scale %d\n",currentScale);
    Fret_clearFrets(fretCtx);
    float basePitch = fmodf(0+currentBase, 12.0);
    //printf("set base %f\n", basePitch);
    Fret_placeFret(fretCtx, basePitch, 3);
    for(int r=0; r<RINGS; r++)
    {
        for(int f=0; f<MAXFRETS; f++)
        {
            if(etPicked[f][r][currentScale])
            {
                float pitch = fmod(12.0 * f / (1.0*ets[r]) + currentBase, 12.0);
                //printf("add fret %f\n",pitch);
                Fret_placeFret(fretCtx, pitch,etPicked[f][r][currentScale]);
            }
        }
    }
    //printf("commited scale %d\n",currentScale);    
}

void ScaleControl_toggle(void* ctx)
{
    if(fret)
    {
        etPicked[fret][ring][currentScale] = !etPicked[fret][ring][currentScale];        
    }
    ScaleControl_commit(ctx);
}

void ScaleControl_setCurrentScale(int currentScaleArg)
{
    //printf("currentScale=%d\n",currentScale);
    currentScale = currentScaleArg;
    ScaleControl_commit(NULL);
}

void ScaleControl_defaults(void*ctx)
{
    //Diatonic minor
    etPicked[0][1][0] = 4;
    etPicked[2][1][0] = 3;
    etPicked[3][1][0] = 3;
    etPicked[5][1][0] = 3;
    etPicked[7][1][0] = 3;
    etPicked[8][1][0] = 3;
    etPicked[10][1][0] = 3;
    
    //Harmonic minor
    etPicked[0][1][1] = 4;
    etPicked[2][1][1] = 3;
    etPicked[3][1][1] = 3;
    etPicked[5][1][1] = 3;
    etPicked[7][1][1] = 3;
    etPicked[8][1][1] = 3;
    etPicked[11][1][1] = 1;
    
    //Chromatic
    etPicked[0][1][2] = 4;
    etPicked[1][1][2] = 1;
    etPicked[2][1][2] = 3;
    etPicked[3][1][2] = 3;
    etPicked[4][1][2] = 1;
    etPicked[5][1][2] = 3;
    etPicked[6][1][2] = 1;
    etPicked[7][1][2] = 3;
    etPicked[8][1][2] = 3;
    etPicked[9][1][2] = 1;
    etPicked[10][1][2] = 3;
    etPicked[11][1][2] = 1;
    
    //Chromatic with quartertones
    etPicked[0][1][3] = 4;
    etPicked[1][1][3] = 2;
    etPicked[3][3][3] = 1;
    etPicked[2][1][3] = 3;
    etPicked[3][1][3] = 3;
    etPicked[4][1][3] = 2;
    etPicked[5][1][3] = 3;
    etPicked[6][1][3] = 2;
    etPicked[13][3][3] = 1;
    etPicked[7][1][3] = 3;
    etPicked[8][1][3] = 3;
    etPicked[17][3][3] = 1;
    etPicked[9][1][3] = 2;
    etPicked[10][1][3] = 3;
    etPicked[11][1][3] = 2;
    
    //Partial 53ET
    etPicked[0][5][4] = 4;
    etPicked[4][5][4] = 1;
    etPicked[5][5][4] = 1;
    etPicked[9][5][4] = 3;
    etPicked[9+4][5][4] = 1;
    etPicked[9+5][5][4] = 1;
    etPicked[9+5+4][5][4] = 1;
    etPicked[9+5+5][5][4] = 1;
    
    etPicked[22+0][5][4] = 3;
    etPicked[22+4][5][4] = 1;
    etPicked[22+5][5][4] = 1;
    etPicked[22+9][5][4] = 3;
    etPicked[22+9+4][5][4] = 1;
    etPicked[22+9+5][5][4] = 1;
    etPicked[22+9+5+4][5][4] = 1;
    etPicked[22+9+5+5][5][4] = 1;
    
    etPicked[44][5][4] = 3;
    etPicked[44+1][5][4] = 1;
    etPicked[44+4][5][4] = 1;
    etPicked[44+5][5][4] = 1;
    
    
    //full 53et
    for(int i=0;i<53;i++)
    {
        etPicked[i][5][5] = 1;
    }  
    etPicked[0][5][5] = 4;
    //full 31et
    for(int i=0;i<31;i++)
    {
        etPicked[i][4][6] = 1;
    }
    etPicked[0][4][6] = 4;
    //full 19et
    for(int i=0;i<19;i++)
    {
        etPicked[i][2][7] = 1;
    }
    etPicked[0][2][7] = 4;
    
    ScaleControl_setCurrentScale(2);
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

void ScaleControl_setBaseNote(int currentBaseArg)
{
    //printf("baseNote=%d\n",currentBaseArg);
    currentBase = currentBaseArg;
    ScaleControl_commit(NULL);
}

int ScaleControl_getBaseNote()
{
    return currentBase;
}



int ScaleControl_getCurrentScale()
{
    return currentScale;
}

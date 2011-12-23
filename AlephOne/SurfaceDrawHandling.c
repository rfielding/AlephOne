//
//  SurfaceDrawHandling.c
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "SurfaceDrawHandling.h"
#include "VertexObjectBuilder.h"
#include "PitchHandler.h"
#include "SurfaceTouchHandling.h"
#include "WidgetTree.h"

#define NULL ((void*)0)
static int triangles;
static int trianglestrip;
static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;


void SurfaceDraw_init(
                          struct VertexObjectBuilder* voCtxDynamicArg,
                          struct PitchHandler_context* phctxArg,
                          int trianglesArg,
                          int trianglestripArg
                      )
{
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    phctx = phctxArg;
    voCtxDynamic = voCtxDynamicArg;
}

//This is always widget 0 with these bounds!
struct WidgetTree_rect* SurfaceDraw_create()
{
    struct WidgetTree_rect* itemP = NULL;
    int widget=0;    
    itemP = WidgetTree_add(widget,0,0,1,1);
    itemP->render = SurfaceDraw_render;
    itemP->up = SurfaceTouchHandling_touchesUp;
    itemP->down = SurfaceTouchHandling_touchesDown;
    itemP->tick = SurfaceTouchHandling_tick;
    itemP->flush = SurfaceTouchHandling_touchesFlush;   
    return itemP;
}

void drawBackground()
{    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, 0, 0, 0,0,0,255);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, 1, 0, 0,0,0,255);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, 0, 0, 0,0,0,255);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, 1, 0, 0,0,0,255);
}


void drawMoveableFrets()
{
    float pitch=0;
    float x=0;
    float y=0;
    float dx = 0.01;
    float dy = 1.0/PitchHandler_getRowCount(phctx);
    int importance=1;
    float usage;
    int ourFret;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic, triangles);
    
    PitchHandler_getFretsBegin(phctx);
    while(PitchHandler_getFret(phctx,&pitch, &x, &y, &importance, &usage,&ourFret))
    {
        float dxi = dx*importance*(1+usage);
        float dyi = dy*(1+importance*0.1);
        float bCol = importance * 255.0 / 4.0;
        
        int red = 0;
        int green = usage*50;
        int blue = bCol;
        int trans = 200;
        
        int rede = 0;
        int greene = 255;
        int bluee = 255;
        int transe = 0;
        
        if(importance == 3)
        {
            red = 200;
        }
        else
        {
            if(importance == 1)
            {
                green = 255;  
                trans = 64+usage*64;
            }            
        }
        
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y    ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x-dxi, y    ,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y-dyi,0,rede,greene,bluee,transe);            
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y    ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y+dyi,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x+dxi, y    ,0,rede,greene,bluee,transe);            
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y    ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y+dyi,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x-dxi, y    ,0,rede,greene,bluee,transe);            
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y    ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x+dxi, y    ,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y-dyi,0,rede,greene,bluee,transe);            
    }   
    
    
}




void drawFingerLocation()
{
    float dx = 0.05;
    float dy = 0.3;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic, triangles);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(phctx,f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX-dx, fInfo->fingerY   ,0,255,127,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY-dy,0,255,127,127,  0);            
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY+dy,0,255,127,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX+dx, fInfo->fingerY   ,0,255,127,127,  0);            
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY+dy,0,255,127,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX-dx, fInfo->fingerY   ,0,255,127,127,  0);            
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX+dx, fInfo->fingerY   ,0,255,127,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY-dy,0,255,127,127,  0);            
            
        }
    }     
}

void drawPitchLocation()
{
    float dx = 0.05;
    float dy = 0.3;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic, triangles);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(phctx,f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX-dx, fInfo->pitchY   ,0,127,255,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY-dy,0,127,255,127,  0);            
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY+dy,0,127,255,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX+dx, fInfo->pitchY   ,0,127,255,127,  0);            
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY+dy,0,127,255,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX-dx, fInfo->pitchY   ,0,127,255,127,  0);            
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX+dx, fInfo->pitchY   ,0,127,255,127,  0);            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY-dy,0,127,255,127,  0);            
            
        }
    }    
}

void SurfaceDraw_render(void* ctx)
{    
    drawBackground();    
    drawMoveableFrets();
    drawFingerLocation();
    drawPitchLocation();
}

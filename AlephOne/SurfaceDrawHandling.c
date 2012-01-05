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
#include "WidgetConstants.h"

#define NULL ((void*)0)
static int triangles;
static int trianglestrip;
static int linestrip;
static struct VertexObjectBuilder* voCtxStatic;
static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;

static int firstDraw = 0;


void SurfaceDraw_init(
                          struct VertexObjectBuilder* voCtxDynamicArg,
                          struct VertexObjectBuilder* voCtxStaticArg,
                          struct PitchHandler_context* phctxArg,
                          int trianglesArg,
                          int trianglestripArg,
                          int linestripArg
                      )
{
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    linestrip = linestripArg;
    phctx = phctxArg;
    voCtxDynamic = voCtxDynamicArg;
    voCtxStatic = voCtxStaticArg;
}

//This is always widget 0 with these bounds!
struct WidgetTree_rect* SurfaceDraw_create()
{
    struct WidgetTree_rect* itemP = NULL;
    itemP = WidgetTree_add(0,0,1,1);
    itemP->render = SurfaceDraw_render;
    itemP->up = SurfaceTouchHandling_touchesUp;
    itemP->down = SurfaceTouchHandling_touchesDown;
    itemP->tick = SurfaceTouchHandling_tick;
    itemP->flush = SurfaceTouchHandling_touchesFlush;   
    return itemP;
}

void SurfaceDraw_drawBackground()
{    
    VertexObjectBuilder_reset(voCtxStatic);
    VertexObjectBuilder_startTexturedObject(voCtxStatic,trianglestrip,PIC_ALEPHONE);
    VertexObjectBuilder_addTexturedVertex(voCtxStatic, 0.1+0.00, 0.05+0.00, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxStatic, 0.1+0.00, 0.05+0.25, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxStatic, 0.1+0.20, 0.05+0.00, 0, 1,0);
    VertexObjectBuilder_addTexturedVertex(voCtxStatic, 0.1+0.20, 0.05+0.25, 0, 1,1);    
    VertexObjectBuilder_startColoredObject(voCtxStatic,trianglestrip);
    VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, 0, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, 1, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, 0, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, 1, 0, 0,0,0,200);    
    
    float cols = PitchHandler_getColCount(phctx);
    float rows = PitchHandler_getRowCount(phctx);
    float dy = 1.0 / rows;
    float ds = dy/2;
    VertexObjectBuilder_startColoredObject(voCtxStatic,triangles);
    
    for(float f=ds; f<1.0; f+=dy)
    {        
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, f+dy, 0, 0,  0,  0,  0);        
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, f+dy, 0, 0,  0,  0,  0);      
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, f+dy, 0, 0,  0,  0,  0);
        
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, f-dy, 0, 0,  0,  0,  0);        
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, f-dy, 0, 0,  0,  0,  0);      
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxStatic, 1, f-dy, 0, 0,  0,  0,  0);
    }
    for(float f=ds; f<1.0; f+=dy)
    {
        float dx = 0.023;
        float iy = 0.023;
        int stdNoteBase = ((int)PitchHandler_findStandardNote(phctx,0.5/cols,f));
        for(int c=0; c<cols; c++)
        {            
            float x = (1.0*c + 0.5)/cols;
            int stdNote = (stdNoteBase + c)%12;
            int r;
            int g;
            int b;
            int isNatural = 0;
            switch(stdNote)
            {
                case 0:
                case 2:
                case 4:
                case 5:
                case 7:
                case 9:
                case 11:
                    r=0;
                    g=255;
                    b=255;
                    isNatural = 1;
                    break;
                default:
                    r=0;
                    g=0;
                    b=0;
            }
            VertexObjectBuilder_startTexturedObject(voCtxStatic,trianglestrip,(PIC_NOTE0+stdNote));
            VertexObjectBuilder_addTexturedVertex(voCtxStatic, x-dx, f-iy, 0, 0,0);
            VertexObjectBuilder_addTexturedVertex(voCtxStatic, x-dx, f+iy, 0, 0,1);
            VertexObjectBuilder_addTexturedVertex(voCtxStatic, x+dx, f-iy, 0, 1,0);
            VertexObjectBuilder_addTexturedVertex(voCtxStatic, x+dx, f+iy, 0, 1,1);
        }
    }
     //*/
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
        float dxi = dx*importance*(1+usage)*0.5;
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
    if(firstDraw == 0)
    {
        SurfaceDraw_drawBackground();    
        firstDraw = 1;
    }
    drawMoveableFrets();
    drawFingerLocation();
    drawPitchLocation();
}

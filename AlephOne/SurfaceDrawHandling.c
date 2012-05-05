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
#include <math.h>
#include "FretlessCommon.h"

static int triangles;
static int trianglestrip;
static int lines;
static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;


void SurfaceDraw_init(
                          struct VertexObjectBuilder* voCtxDynamicArg,
                          struct PitchHandler_context* phctxArg,
                          int trianglesArg,
                          int trianglestripArg,
                          int linesArg
                      )
{
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    lines = linesArg;
    phctx = phctxArg;
    voCtxDynamic = voCtxDynamicArg;
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
    //Draw the app logo
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,PIC_ALEPHONE);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0.1+0.00, 0.05+0.00, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0.1+0.00, 0.05+0.25, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0.1+0.20, 0.05+0.00, 0, 1,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0.1+0.20, 0.05+0.25, 0, 1,1);    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, 0, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, 1, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, 0, 0, 0,0,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, 1, 0, 0,0,0,200);    
    
    float cols = PitchHandler_getColCount(phctx);
    float rows = PitchHandler_getRowCount(phctx);
    float dy = 1.0 / rows;
    float ds = dy/2;
    VertexObjectBuilder_startColoredObject(voCtxDynamic,triangles);
    
    //Draw strings
    float dyd = dy*0.5;
    for(float f=ds; f<1.0; f+=dy)
    {        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, f+dyd, 0, 0,  50,  50,  40);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, f+dyd, 0, 0,  50,  50,  40);      
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, f+dyd, 0, 0,  50,  50,  40);
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, f-dyd, 0, 0,  50,  50,  40);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, f-dyd, 0, 0,  50,  50,  40);      
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 0, f   , 0, 0,255,255, 40);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic, 1, f-dyd, 0, 0,  50,  50,  40);
    }
    
    float dx = 0.05;
    float iy = 0.05;
    //Start at bottom corner, increment a chromatic until we are off the screen,
    //then move to next string and detune 
    //and repeat until we are out of strings.
    float bottomLeftNote = PitchHandler_findStandardNote(phctx,0,0);
    //use <0.5, 0.5> as starting point
    float y = 0.5 / rows;
    float x = 0.5 / cols;
    int stdNote = 0;
    float cumulativeDetune = 0;
    int str = 0;
    //Traverse strings
    while(y<1.0)
    {
        //Traverse frets
        while(x<1.0)
        {
            stdNote = ((int)(x*cols + bottomLeftNote + cumulativeDetune))%12;
            //Given the standard note name, draw it
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
            VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,(PIC_NOTE0+stdNote));
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x-dx, y-iy, 0, 0,0);
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x-dx, y+iy, 0, 0,1);
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x+dx, y-iy, 0, 1,0);
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, x+dx, y+iy, 0, 1,1);
            //move up a fret
            x += (1.0)/cols;
        }
        float detune = PitchHandler_getStrDetune(phctx,str);
        cumulativeDetune += detune;
        //Move up a string
        y += 1.0/rows;
        str = (int)(y*rows);
        //Get the incremental detune and move back x as far as we must
        do
        {
            x -= detune/cols;        
        }while(x > 0);
    }
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
        int trans = 180;
        
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
    float dx = 0.0125;
    float dy = 0.3;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic, triangles);
    for(int f=0; f<FINGERMAX; f++)
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
    float dx = 0.0125;
    float dy = 0.3;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic, triangles);
    for(int f=0; f<FINGERMAX; f++)
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
    float cols = PitchHandler_getColCount(phctx);
    float rows = PitchHandler_getRowCount(phctx);
    VertexObjectBuilder_startColoredObject(voCtxDynamic, lines);
    for(int f=0; f<FINGERMAX; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(phctx,f);
        if(fInfo->isActive)
        {
            //TODO: this is ONLY right for uniform tunings
            float y = ((int)(rows*fInfo->pitchY) + 0.5)/rows;
            float dy = 1.0/rows;
            float detune;
            float detune2;
            

            if(PitchHandler_getRowCount(phctx)>1.5)
            {
                detune = PitchHandler_getStrDetune(phctx,(int)(fInfo->pitchY*rows));
                float dx3 = (12*log2f(3.0/2) - detune)/cols;
                float dx4 = (12*log2f(4.0/3) - detune)/cols;
                float dx5 = (12*log2f(5.0/4) - detune)/cols;
                float dx6 = (12*log2f(6.0/5) - detune)/cols;
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  0,0, 255,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX+0.75*dx3,    y+0.75*dy,0,  0,0, 255,0);  
                
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  0,0,255,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX+dx4, y+dy,0,  0,0,255,0);      
                
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  0,255, 0,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX+0.75*dx5, y+0.75*dy,0,  0,255, 0,0);    
                
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  50,255, 0,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX+0.75*dx6, y+0.75*dy,0,  50,255, 0,0);            
                
                int bottomRow = ((fInfo->pitchY*rows)>0)==0;
                detune2 = PitchHandler_getStrDetune(phctx,bottomRow + (int)(fInfo->pitchY*rows - 1));                
                float dx32 = (12*log2f(3.0/2) - detune2)/cols;
                float dx42 = (12*log2f(4.0/3) - detune2)/cols;
                float dx52 = (12*log2f(5.0/4) - detune2)/cols;
                float dx62 = (12*log2f(6.0/5) - detune2)/cols;
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  0,0, 255,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX-0.75*dx32,    y-0.75*dy,0,  0,0, 255,0);    
                
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  0,0, 255,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX-dx42, y-dy,0,  0,0,255,0);      
                
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  0,255, 0,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX-0.75*dx52, y-0.75*dy,0,  0,255, 0,0);      
                
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX,    y,0,  50,255, 0,255);            
                VertexObjectBuilder_addColoredVertex(voCtxDynamic,fInfo->pitchX-0.75*dx62, y-0.75*dy,0,  50,255, 0,0);                            
            }
        }
    } 
}

void SurfaceDraw_render(void* ctx)
{    
    SurfaceDraw_drawBackground();    
    drawMoveableFrets();
    drawFingerLocation();
    drawPitchLocation();
}

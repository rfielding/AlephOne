//
//  ObjectRendering.c
//  AlephOne
//
//  Created by Robert Fielding on 12/18/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//
/**
   Look ma!  No OpenGL dependency at all!
 */

#include "VertexObjectBuilder.h"
#include "ObjectRendering.h"
#include "Fretless.h"
#include "PitchHandler.h"

#include <math.h>


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

#define PIC_TUTORIAL 0
#define PIC_ASHMEDI 1
#define PIC_STARS 2
#define PIC_CHANNELCYCLING 3

static unsigned int textures[256];
static float textureWidth[256];
static float textureHeight[256];
static float lightPosition[3];


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

void GenericRendering_drawBackground()
{    
    float left  = 0 + lightPosition[0]*0.1 + 0.2;
    float right = 1 + lightPosition[0]*0.1 - 0.2;
    float top   = 1 + lightPosition[1]*0.1 - 0.2;
    float bottom= 0 + lightPosition[1]*0.1 + 0.2;
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,PIC_STARS);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0, 0, 0, left,bottom);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0, 1, 0, left,top);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 1, 0, 0, right,bottom);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 1, 1, 0, right,top);
}

void drawOccupancyHandle(float cx, float cy, float diameter,float z)
{
    z = z/16 * 2*M_PI;
    
    //Draw the endpoints of the channel cycle
    float rA = (diameter*0.25+0.02);
    float rB = (diameter*0.25);
    float cosA = cosf(z+0.1);
    float sinA = sinf(z+0.1);
    float cosB = cosf(z-0.1);
    float sinB = sinf(z-0.1);
    float cosC = cosf(z);
    float sinC = sinf(z);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rB*sinC,cy+rB*cosC,0,255, 255,255,255);        
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rA*sinA,cy+rA*cosA,0,200, 200,  0,255);        
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rA*sinB,cy+rA*cosB,0,200, 200,  0,255);        
}

void GenericRendering_drawChannelOccupancy(float cx,float cy,float diameter)
{
    //Draw the main radius of the channel cycle
    VertexObjectBuilder_startColoredObject(voCtxDynamic,linestrip);    
    float r = (diameter*0.25);
    for(int channel=0; channel<16; channel++)
    {
        float a = channel/16.0 * 2*M_PI;
        float cosA = cosf(a);
        float sinA = sinf(a);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r*sinA,cy+r*cosA,0,0, 255, 0,255);                
    }
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx,cy+r,0,0, 255, 0,255);  
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,triangles);
    int bottom = Fretless_getMidiHintChannelBase(fctx);
    int top  = (bottom + Fretless_getMidiHintChannelSpan(fctx) + 15)%16;
    drawOccupancyHandle(cx,cy,diameter,bottom);
    drawOccupancyHandle(cx,cy,diameter,top);
    
    //Draw activity in the channel cycle
    for(int channel=0; channel<16; channel++)
    {
        float r = (diameter*0.5) * Fretless_getChannelOccupancy(fctx, channel);
        float a = channel/16.0 * 2*M_PI;
        float cosA = cosf(a-0.15);
        float sinA = sinf(a-0.15);
        float cosB = cosf(a+0.15);
        float sinB = sinf(a+0.15);
        float cosC = cosf(a);
        float sinC = sinf(a);
        
        //Represent the current bend setting for each channel
        float rC = (diameter*0.5)/4;
        float b = Fretless_getChannelBend(fctx, channel);
        float rD = rC + (diameter*0.5/4)*b;
        
        int red   = b>0 ? 255 : 0;
        int green = 0;
        int blue  = b>0 ? 0 : 255;
        
        //Draw the channel cycling
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx,cy,0,0, 255, 0,255);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r*sinA,cy+r*cosA,0,0, 200, 0,  0);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r*sinB,cy+r*cosB,0,0, 255, 0,  0); 
        
        //Draw what the bend manipulation is doing
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rC*sinA,cy+rC*cosA,0,red, green, blue,255);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rC*sinB,cy+rC*cosB,0,red*0.5, green*0.5, blue*0.5,255);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rD*sinC,cy+rD*cosC,0,red*0.5, green*0.5, blue*0.5,255);  
        
    }
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,PIC_CHANNELCYCLING);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx-diameter/2, cy+diameter/8 - diameter/4, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx-diameter/2, cy-diameter/8 - diameter/4, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx+diameter/2, cy+diameter/8 - diameter/4, 0, 1,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx+diameter/2, cy-diameter/8 - diameter/4, 0, 1,1);
}




void GenericRendering_drawMoveableFrets()
{
    float pitch=0;
    float x=0;
    float y=0;
    float dx = 0.01;
    float dy = 1.0/PitchHandler_getRowCount(phctx);
    int importance=1;
    float usage;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic, triangles);
    
    PitchHandler_getFretsBegin(phctx);
    while(PitchHandler_getFret(phctx,&pitch, &x, &y, &importance, &usage))
    {
        float dxi = dx*importance*(1+usage);
        float bCol = importance * 255.0 / 4.0;
        
        int red = 0;
        int green = usage*50;
        int blue = 255;
        int trans = 255;
        
        int rede = 255;
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
        
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y   ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x-dxi, y   ,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y-dy,0,rede,greene,bluee,transe);            
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y   ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y+dy,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x+dxi, y   ,0,rede,greene,bluee,transe);            
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y   ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y+dy,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x-dxi, y   ,0,rede,greene,bluee,transe);            
        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x,     y   ,0,red,green,blue,trans);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x+dxi, y   ,0,rede,greene,bluee,transe);            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,x    , y-dy,0,rede,greene,bluee,transe);            
    }   
    
    
}




void GenericRendering_drawFingerLocation()
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

void GenericRendering_drawPitchLocation()
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

void GenericRendering_dynamic()
{
    VertexObjectBuilder_reset(voCtxDynamic);    
    
    GenericRendering_drawBackground();    
    GenericRendering_drawMoveableFrets();
    GenericRendering_drawFingerLocation();
    GenericRendering_drawPitchLocation();
    
    GenericRendering_drawChannelOccupancy(0.5, 0.5, 0.6);
    
}

void GenericRendering_draw()
{
    GenericRendering_dynamic();
    ObjectRendering_drawVO(voCtxDynamic);    
}
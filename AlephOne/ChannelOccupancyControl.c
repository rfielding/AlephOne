//
//  ChannelOccupancyControl.c
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "ChannelOccupancyControl.h"
#include "PitchHandler.h"
#include "VertexObjectBuilder.h"
#include <math.h>
#include "WidgetTree.h"
#include "Fretless.h"

#include "WidgetConstants.h"
#include <stdlib.h>

static int triangles;
static int trianglestrip;
static int linestrip;
static struct VertexObjectBuilder* voCtxDynamic;
static struct Fretless_context* fctx;

void ChannelOccupancyControl_touchesInit(
                                      int trianglesArg,
                                      int trianglestripArg,
                                      int linestripArg,
                                      struct VertexObjectBuilder* voCtxDynamicArg, 
                                      struct Fretless_context* fctxArg
                                      )
{
    fctx = fctxArg;
    voCtxDynamic = voCtxDynamicArg;
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    linestrip = linestripArg;
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
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rA*sinA,cy+rA*cosA,0,200, 200,  0,200);        
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rA*sinB,cy+rA*cosB,0,200, 200,  0,200);        
}

void ChannelOccupancyControl_render(void* ctx)
{
    struct WidgetTree_rect* panel = ((struct ChannelOccupancyControl_data*)ctx)->rect;
    int bottom = Fretless_getMidiHintChannelBase(fctx);
    int span = Fretless_getMidiHintChannelSpan(fctx);
    int bend = Fretless_getMidiHintChannelBendSemis(fctx);
    int top  = (bottom + span + 15)%16;
    
    float cx = (panel->x1 + panel->x2)/2;
    float cy = (panel->y1 + panel->y2)/2;
    float diameter = (panel->x2 - panel->x1);
    
    //Draw the main radius of the channel cycle
    float r = (diameter*0.25);
    float r2 = r + diameter*0.05;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);    
    for(int channel=bottom; channel<(bottom+span); channel++)
    {
        float a = channel/16.0 * 2*M_PI;
        float cosA = cosf(a);
        float sinA = sinf(a);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r*sinA,cy+r*cosA,0,0, 127, 50,100);                            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r2*sinA,cy+r2*cosA,0,0, 127, 50,0);                            
    }
    
    float rLo = r/2 - (r/2)/bend;
    float rHi = r/2 + (r/2)/bend;
    VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);    
    for(int channel=bottom; channel<(bottom+span+1); channel++)
    {
        float a = channel/16.0 * 2*M_PI;
        float cosA = cosf(a);
        float sinA = sinf(a);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rLo*sinA,cy+rLo*cosA,0, 0, 0, 200,75);                            
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rHi*sinA,cy+rHi*cosA,0,200, 0,  0,75);                            
    }    
    //VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx,cy+r,0,0, 255, 0,127);  
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,triangles);
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
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,PIC_CHANNELCYCLINGTEXT);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx-diameter/2 + diameter/8, cy+diameter/8 - diameter/4, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx-diameter/2 + diameter/8, cy-diameter/8 - diameter/4, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx+diameter/2 + diameter/8, cy+diameter/8 - diameter/4, 0, 1,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, cx+diameter/2 + diameter/8, cy-diameter/8 - diameter/4, 0, 1,0);
}

struct ChannelOccupancyControl_data* ChannelOccupancyControl_create(float x1,float y1, float x2,float y2)
{
    struct ChannelOccupancyControl_data* coControl = 
        (struct ChannelOccupancyControl_data*)malloc(sizeof(struct ChannelOccupancyControl_data));
    
    coControl->rect = WidgetTree_add(x1,y1,x2,y2);    
    coControl->rect->render = ChannelOccupancyControl_render;   
    coControl->rect->ctx = coControl;
    return coControl;
}

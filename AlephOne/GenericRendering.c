//
//  GenericRendering.c
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "GenericRendering.h"
#include "PitchHandler.h"
#include "Fretless.h"
#include "VertexObjectBuilder.h"
#include "Transforms.h"
#include <OpenGLES/ES1/gl.h>
#include <stdlib.h>
#include <math.h>

#include <stdio.h>

struct VertexObjectBuilder* voCtxDynamic;
struct PitchHandler_context* phctx;
struct Fretless_context* fctx;



static float lightPosition[] = {0, 0, -1,0};
//static float specularAmount[] = {0.0,0.0,0.0,1.0};
//static float diffuseAmount[] = {1.0,0.8,1.0,1.0};
//static float ambientAmount[] = {1.0,1.0,1.0,1.0};

static float scale[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static char* requiredTexture[] = {
    "tutorial",
    "ashmedi",
    "tutorial"
};

#define PIC_TUTORIAL 0
#define PIC_MOLOCH 1
#define PIC_ASHMEDI 2

unsigned int textures[256];

void GenericRendering_init(struct PitchHandler_context* phctxArg,struct Fretless_context* fctxArg)
{
    phctx = phctxArg;
    fctx  = fctxArg;
}

void GenericRendering_setup()
{
    voCtxDynamic = VertexObjectBuilder_init(malloc,printf);    
}

char* GenericRendering_getRequiredTexture(int idx)
{
    return requiredTexture[idx];
}

void  GenericRendering_assignRequiredTexture(int idx,int val)
{
    printf("textures[%d] = %d\n",idx,val);
    textures[idx] = val;
}

void GenericRendering_updateLightOrientation(float x,float y, float z)
{
    lightPosition[0] = x;
    lightPosition[1] = y;
    lightPosition[2] = z;
}

void GenericRendering_camera()
{
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    Transforms_getOrientation(scale);
    
    glMultMatrixf(scale);
    glScalef(2,2,1);
    glTranslatef(-0.5,-0.5,0);    
} 

void GenericRendering_drawBackground()
{    
    float lx=0;//lightPosition[0]+64;
    float ly=0;//lightPosition[1]+64;
    float lz=0;//lightPosition[2]+64;
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,GL_TRIANGLE_STRIP);
    
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,0,0,0,lx, ly, lz,255);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,1,0,0,lz, lx, ly,255);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,0,1,0,ly, lz, lx,255); 
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,1,1,0,lx, lz, ly,255); 
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
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rB*sinC,cy+rB*cosC,0,255, 255,255,127);        
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rA*sinA,cy+rA*cosA,0,200, 200,  0,100);        
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rA*sinB,cy+rA*cosB,0,200, 200,  0,100);        
}

void GenericRendering_drawChannelOccupancy(float cx,float cy,float diameter)
{
    //Draw the main radius of the channel cycle
    VertexObjectBuilder_startColoredObject(voCtxDynamic,GL_LINE_STRIP);    
    float r = (diameter*0.25);
    for(int channel=0; channel<16; channel++)
    {
        float a = channel/16.0 * 2*M_PI;
        float cosA = cosf(a);
        float sinA = sinf(a);
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r*sinA,cy+r*cosA,0,0, 255, 0,64);                
    }
    VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx,cy+r,0,0, 255, 0,64);  
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,GL_TRIANGLES);
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
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx,cy,0,0, 255, 0,127);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r*sinA,cy+r*cosA,0,0, 200, 0,  0);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+r*sinB,cy+r*cosB,0,0, 255, 0,  0); 
        
        //Draw what the bend manipulation is doing
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rC*sinA,cy+rC*cosA,0,red, green, blue,200);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rC*sinB,cy+rC*cosB,0,red*0.5, green*0.5, blue*0.5,200);        
        VertexObjectBuilder_addColoredVertex(voCtxDynamic,cx+rD*sinC,cy+rD*cosC,0,red*0.5, green*0.5, blue*0.5,200);  
        
    }
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
     
    VertexObjectBuilder_startColoredObject(voCtxDynamic, GL_TRIANGLES);
    
    PitchHandler_getFretsBegin(phctx);
    while(PitchHandler_getFret(phctx,&pitch, &x, &y, &importance, &usage))
    {
        float dxi = dx*importance*(1+usage);
        float bCol = importance * 255.0 / 4.0;
        
        int red = 0;
        int green = usage*50;
        int blue = 255;
        int trans = bCol;
        
        int rede = 127;
        int greene = 127;
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

    VertexObjectBuilder_startColoredObject(voCtxDynamic, GL_TRIANGLES);
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

    VertexObjectBuilder_startColoredObject(voCtxDynamic, GL_TRIANGLES);
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

void testImage()
{
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,GL_TRIANGLE_STRIP,PIC_ASHMEDI);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0, 0, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0.1, 0, 0, 1,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0, 0.1, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, 0.1, 0.1, 0, 1,1);
}

void GenericRendering_dynamic()
{
    VertexObjectBuilder_reset(voCtxDynamic);    

    GenericRendering_drawBackground();    
    GenericRendering_drawMoveableFrets();
    GenericRendering_drawFingerLocation();
    GenericRendering_drawPitchLocation();
    
    GenericRendering_drawChannelOccupancy(0.8, 0.8, 0.4);
    
    testImage();
}

void GenericRendering_drawVO(struct VertexObjectBuilder* vobj)
{    
    int voCount = VertexObjectBuilder_getVertexObjectsCount(vobj);

    for(int o=0; o<voCount;o++)
    {
        struct VertexObject* vo = VertexObjectBuilder_getVertexObject(vobj,o);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vo->vertices);    
        
        if(vo->usingColor)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnableClientState(GL_COLOR_ARRAY);            
            glColorPointer(4, GL_UNSIGNED_BYTE, 0, vo->colors);            
        }
        else
        {
            glDisableClientState(GL_COLOR_ARRAY);
        }
        
        if(vo->usesNormals)
        {
            glEnableClientState(GL_NORMAL_ARRAY);            
            glNormalPointer(GL_FLOAT,0, vo->normals);            
        }
        else
        {
            glDisableClientState(GL_NORMAL_ARRAY);                        
        }
        
        if(vo->usingTexture)
        {
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            float tex[] = {
                0,0,
                0,1,
                1,0,
                1,1
            };
            glTexCoordPointer(2,GL_FLOAT,0, tex);  
            int texture = textures[vo->textureId];
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); 
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        }
        else
        {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }
        
        glDrawArrays(vo->type, 0, vo->count);            
    }
}

void GenericRendering_draw()
{
    GenericRendering_dynamic();
    GenericRendering_drawVO(voCtxDynamic);    
}
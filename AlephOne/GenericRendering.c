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

//#include <stdio.h>

struct VertexObjectBuilder* voCtxStatic;
struct VertexObjectBuilder* voCtxDynamic;
struct PitchHandlerContext* phctx;
struct Fretless_context* fctx;

static float lightPosition[] = {0, 0, -1,0};
static float specularAmount[] = {0.0,0.0,0.0,1.0};
static float diffuseAmount[] = {1.0,0.8,1.0,1.0};
static float ambientAmount[] = {1.0,1.0,1.0,1.0};

static float scale[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void GenericRendering_init(struct PitchHandlerContext* phctxArg,struct Fretless_context* fctxArg)
{
    phctx = phctxArg;
    fctx  = fctxArg;
}

void GenericRendering_updateLightOrientation(float x,float y, float z)
{
    lightPosition[0] = x;
    lightPosition[1] = y;
    lightPosition[2] = z;
}

void GenericRendering_camera()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    Transforms_getOrientation(scale);
    
    glMultMatrixf(scale);
    glScalef(2,2,1);
    glTranslatef(-0.5,-0.5,0);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ///*
    //glEnable(GL_LIGHTING);
    
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseAmount);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientAmount);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularAmount);
    
     //*/ 
    
//    glClearColor(0.9,0.8,0.3,1.0);
} 

void GenericRendering_drawBackground()
{
    //int rows = PitchHandler_getRowCount(phctx);
    //int cols = PitchHandler_getColCount(phctx);
    
    //float xscale = 1.0/cols;
    //float yscale = 1.0/rows;
    //float halfXscale = 0.5*xscale;
    //float halfYscale = 0.5*yscale;
    
    float lx=64*lightPosition[0]+64;
    float ly=64*lightPosition[1]+64;
    float lz=64*lightPosition[2]+64;
    
    VertexObjectBuilder_startObject(voCtxStatic,GL_TRIANGLE_STRIP);
    
    VertexObjectBuilder_addVertex(voCtxStatic,0,0,0, 
                                  lx, ly, lz,
                                  255,0,0,1);
    VertexObjectBuilder_addVertex(voCtxStatic,1,0,0, 
                                  lz, lx, ly,
                                  255,0,0,1);
    VertexObjectBuilder_addVertex(voCtxStatic,0,1,0, 
                                  ly, lz, lx,
                                  255,0,0,1);
    VertexObjectBuilder_addVertex(voCtxStatic,1,1,0, 
                                  lx, lz, ly,
                                  255,0,0,1);
}

void drawOccupancyHandle(float cx, float cy, float diameter,float z)
{
    z = z/16 * 2*M_PI;
    
    //Draw the endpoints of the channel cycle
    float rA = (diameter*0.25+0.02);
    float rB = (diameter*0.25-0.02);
    float cosA = cosf(z+0.1);
    float sinA = sinf(z+0.1);
    float cosB = cosf(z-0.1);
    float sinB = sinf(z-0.1);
    float cosC = cosf(z);
    float sinC = sinf(z);
    VertexObjectBuilder_addVertex(voCtxStatic,cx+rB*cosC,cy+rB*sinC,0, 
                                  0, 255,255,127,0,0,1);        
    VertexObjectBuilder_addVertex(voCtxStatic,cx+rA*cosA,cy+rA*sinA,0, 
                                  0, 200,  0,100,0,0,1);        
    VertexObjectBuilder_addVertex(voCtxStatic,cx+rA*cosB,cy+rA*sinB,0, 
                                  0, 200,  0,100,0,0,1);        
}

void GenericRendering_drawChannelOccupancy(float cx,float cy,float diameter)
{
    //Draw the main radius of the channel cycle
    VertexObjectBuilder_startObject(voCtxStatic,GL_LINE_STRIP);    
    float r = (diameter*0.25);
    for(int channel=0; channel<16; channel++)
    {
        float a = channel/16.0 * 2*M_PI;
        float cosA = cosf(a);
        float sinA = sinf(a);
        VertexObjectBuilder_addVertex(voCtxStatic,cx+r*cosA,cy+r*sinA,0, 
                                      0, 255, 0,64,0,0,1);                
    }
    VertexObjectBuilder_addVertex(voCtxStatic,cx+r,cy,0, 
                                  0, 255, 0,64,0,0,1);  
    
    VertexObjectBuilder_startObject(voCtxStatic,GL_TRIANGLES);
    int bottom = Fretless_getMidiHintChannelBase(fctx);
    int top  = (bottom + Fretless_getMidiHintChannelSpan(fctx) + 15)%16;
    drawOccupancyHandle(cx,cy,diameter,bottom);
    drawOccupancyHandle(cx,cy,diameter,top);
    
    //Draw activity in the channel cycle
    for(int channel=0; channel<16; channel++)
    {
        float r = (diameter*0.5) * Fretless_getChannelOccupancy(fctx, channel);
        float a = channel/16.0 * 2*M_PI;
        float cosA = cosf(a-0.1);
        float sinA = sinf(a-0.1);
        float cosB = cosf(a+0.1);
        float sinB = sinf(a+0.1);
        VertexObjectBuilder_addVertex(voCtxStatic,cx,cy,0, 
                                      0, 255, 0,127,0,0,1);        
        VertexObjectBuilder_addVertex(voCtxStatic,cx+r*cosA,cy+r*sinA,0, 
                                      0, 255, 0,  0,0,0,1);        
        VertexObjectBuilder_addVertex(voCtxStatic,cx+r*cosB,cy+r*sinB,0, 
                                      0, 255, 0,  0,0,0,1); 
    }
}
void GenericRendering_setup()
{
    voCtxDynamic = VertexObjectBuilder_init(malloc);    
    voCtxStatic = VertexObjectBuilder_init(malloc);
    //GenericRendering_drawBackground();
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
    //float lx=127*lightPosition[0]+127;
    //float ly=127*lightPosition[1]+127;
    //float lz=127*lightPosition[2]+127;
     
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    
    PitchHandler_getFretsBegin(phctx);
    while(PitchHandler_getFret(phctx,&pitch, &x, &y, &importance, &usage))
    {
        float dxi = dx*importance*(1+usage);
        VertexObjectBuilder_addVertex(voCtxDynamic,x,     y   ,0,0,  usage*50,  255,255,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x-dxi, y   ,0,127,127,255,  0,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x    , y-dy,0,127,127,255,  0,0,0,1);            
        
        VertexObjectBuilder_addVertex(voCtxDynamic,x,     y   ,0,  0,   usage*50,255,255,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x    , y+dy,0,127,127,255,  0,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x+dxi, y   ,0,127,127,255,  0,0,0,1);            
        
        VertexObjectBuilder_addVertex(voCtxDynamic,x,     y   ,0,  0,   usage*50,255,255,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x    , y+dy,0,127,127,255,  0,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x-dxi, y   ,0,127,127,255,  0,0,0,1);            
        
        VertexObjectBuilder_addVertex(voCtxDynamic,x,     y   ,0,  0,   usage*50,255,255,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x+dxi, y   ,0,127,127,255,  0,0,0,1);            
        VertexObjectBuilder_addVertex(voCtxDynamic,x    , y-dy,0,127,127,255,  0,0,0,1);            
    }   
    
   
}




void GenericRendering_drawFingerLocation()
{
    float dx = 0.05;
    float dy = 0.3;
    //float lx=127*lightPosition[0]+127;
    //float ly=127*lightPosition[1]+127;
    //float lz=127*lightPosition[2]+127;
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(phctx,f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX-dx, fInfo->fingerY   ,0,255,127,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY-dy,0,255,127,127,  0,0,0,1);            
            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY+dy,0,255,127,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX+dx, fInfo->fingerY   ,0,255,127,127,  0,0,0,1);            
            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY+dy,0,255,127,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX-dx, fInfo->fingerY   ,0,255,127,127,  0,0,0,1);            
            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX,    fInfo->fingerY   ,0,255,  0,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX+dx, fInfo->fingerY   ,0,255,127,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX   , fInfo->fingerY-dy,0,255,127,127,  0,0,0,1);            
            
        }
    }     
}

void GenericRendering_drawPitchLocation()
{
    float dx = 0.05;
    float dy = 0.3;
    //float lx=127*lightPosition[0]+127;
    //float ly=127*lightPosition[1]+127;
    //float lz=127*lightPosition[2]+127;
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(phctx,f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX-dx, fInfo->pitchY   ,0,127,255,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY-dy,0,127,255,127,  0,0,0,1);            

            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY+dy,0,127,255,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX+dx, fInfo->pitchY   ,0,127,255,127,  0,0,0,1);            
            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY+dy,0,127,255,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX-dx, fInfo->pitchY   ,0,127,255,127,  0,0,0,1);            
            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX,    fInfo->pitchY   ,0,  0,255,  0,255,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX+dx, fInfo->pitchY   ,0,127,255,127,  0,0,0,1);            
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX   , fInfo->pitchY-dy,0,127,255,127,  0,0,0,1);            
            
        }
    }    
}

void GenericRendering_dynamic()
{
    VertexObjectBuilder_reset(voCtxStatic);
    GenericRendering_drawBackground();
    GenericRendering_drawChannelOccupancy(0.5, 0.5, 1.0);
    
    VertexObjectBuilder_reset(voCtxDynamic);    
    GenericRendering_drawMoveableFrets();
    GenericRendering_drawFingerLocation();
    GenericRendering_drawPitchLocation();
}

void GenericRendering_drawVO(struct VertexObjectBuilder* vobj)
{
    int voCount = VertexObjectBuilder_getVertexObjectsCount(vobj);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularAmount );
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuseAmount );
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambientAmount );
    for(int o=0; o<voCount;o++)
    {
        struct VertexObject* vo = VertexObjectBuilder_getVertexObject(vobj,o);
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, vo->vertices);        
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, vo->colors);
        glNormalPointer(GL_FLOAT,0, vo->normals);
        
        glDrawArrays(vo->type, 0, vo->count);            
    }
}

void GenericRendering_draw()
{
    GenericRendering_dynamic();
    GenericRendering_drawVO(voCtxStatic);
    GenericRendering_drawVO(voCtxDynamic);
}
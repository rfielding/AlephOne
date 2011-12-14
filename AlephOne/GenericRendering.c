//
//  GenericRendering.c
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "GenericRendering.h"
#include "PitchHandler.h"
#include "VertexObjectBuilder.h"
#include "Transforms.h"
#include <OpenGLES/ES1/gl.h>
#include <stdlib.h>

//#include <stdio.h>

struct VertexObjectBuilder* voCtxStatic;
struct VertexObjectBuilder* voCtxDynamic;
struct PitchHandlerContext* phctx;

static float lightPosition[] = {0, 0, -1,0};
static float specularAmount[] = {0.7,1.0,1.0,1.0};
static float diffuseAmount[] = {1.0,0.8,1.0,1.0};
static float ambientAmount[] = {1.0,1.0,0.9,1.0};

static float scale[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void GenericRendering_init(struct PitchHandlerContext* phctxArg)
{
    phctx = phctxArg;
}

void GenericRendering_updateLightOrientation(float x,float y, float z)
{
    lightPosition[0] = 5*x;
    lightPosition[1] = 5*y;
    lightPosition[2] = 5*z;
    //printf("<%f,%f,%f>\n",x,y,z);
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
//    glEnable(GL_LIGHTING);
    
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
    int rows = PitchHandler_getRowCount(phctx);
    int cols = PitchHandler_getColCount(phctx);
    
    float xscale = 1.0/cols;
    float yscale = 1.0/rows;
    float halfXscale = 0.5*xscale;
    float halfYscale = 0.5*yscale;
    
    VertexObjectBuilder_startObject(voCtxStatic,GL_TRIANGLE_STRIP);
    
    VertexObjectBuilder_addVertex(voCtxStatic,0,0,0, 0,0,0,255,0,0,1);
    VertexObjectBuilder_addVertex(voCtxStatic,1,0,0, 255,0,0,255,0,0,1);
    VertexObjectBuilder_addVertex(voCtxStatic,0,1,0, 0,255,0,255,0,0,1);
    VertexObjectBuilder_addVertex(voCtxStatic,1,1,0, 255,255,0,255,0,0,1);
    
    VertexObjectBuilder_startObject(voCtxStatic,GL_LINES);
    for(int r=0; r<rows; r++)
    {
        for(int c=0; c<cols; c++)
        {
            VertexObjectBuilder_addVertex(voCtxStatic,xscale*c + halfXscale,0,0, 0,0,0,255,0,0,1);
            VertexObjectBuilder_addVertex(voCtxStatic,xscale*c + halfXscale,1,0, 0,0,0,255,0,0,1);
        }
        VertexObjectBuilder_addVertex(voCtxStatic,0,yscale*r + halfYscale,0, 0,0,0,255,0,0,1);
        VertexObjectBuilder_addVertex(voCtxStatic,1,yscale*r + halfYscale,0, 0,0,0,255,0,0,1);
    }    
}

void GenericRendering_setup()
{
    voCtxDynamic = VertexObjectBuilder_init(malloc);    
    voCtxStatic = VertexObjectBuilder_init(malloc);
    GenericRendering_drawBackground();
}

void GenericRendering_drawMoveableFrets()
{
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    float pitch=0;
    float x=0;
    float y=0;
    float dx = 0.04;
    float dy = 0.04;
    PitchHandler_getFretsBegin(phctx);
    while(PitchHandler_getFret(phctx,&pitch, &x, &y))
    {
        VertexObjectBuilder_addVertex(voCtxDynamic,x, y - dy,0, 0,0,255,200,0,0,1);
        VertexObjectBuilder_addVertex(voCtxDynamic,x + dx, y + dy,0, 0,0,255,200,0,0,1);
        VertexObjectBuilder_addVertex(voCtxDynamic,x - dx, y + dy,0, 0,0,255,200,0,0,1);    
    }      
}

void GenericRendering_drawFingerLocation()
{
    float dx = 0.035;
    float dy = 0.035;
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(phctx,f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX, fInfo->fingerY - dy,0, 255,0,0,200,0,0,1);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX + dx, fInfo->fingerY + dy,0, 255,0,0,200,0,0,1);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX - dx, fInfo->fingerY + dy,0, 255,0,0,200,0,0,1);            
        }
    }    
}

void GenericRendering_drawPitchLocation()
{
    float dx = 0.03;
    float dy = 0.03;
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(phctx,f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX, fInfo->pitchY - dy,0, 0,255,0,200,0,0,1);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX + dx, fInfo->pitchY + dy,0, 0,255,0,200,0,0,1);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX - dx, fInfo->pitchY + dy,0, 0,255,0,200,0,0,1);            
        }
    }    
}

void GenericRendering_dynamic()
{
    VertexObjectBuilder_reset(voCtxDynamic);
    
    GenericRendering_drawMoveableFrets();
    GenericRendering_drawPitchLocation();
    GenericRendering_drawFingerLocation();
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
    GenericRendering_drawVO(voCtxStatic);
    GenericRendering_dynamic();
    GenericRendering_drawVO(voCtxDynamic);
}
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
#include <OpenGLES/ES1/gl.h>
#include <stdlib.h>

struct VertexObjectBuilder* voCtx;

void GenericRendering_camera()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    static float scale[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    PitchHandler_getOrientation(scale);
    
    glMultMatrixf(scale);
    glScalef(2,2,1);
    glTranslatef(-0.5,-0.5,0);        
}


void GenericRendering_setup()
{
    int rows = PitchHandler_getRowCount();
    int cols = PitchHandler_getColCount();
    
    float xscale = 1.0/cols;
    float yscale = 1.0/rows;
    float halfXscale = 0.5*xscale;
    float halfYscale = 0.5*yscale;
    
    voCtx = VertexObjectBuilder_init(malloc);
    VertexObjectBuilder_startObject(voCtx,GL_TRIANGLE_STRIP);
    
    VertexObjectBuilder_addVertex(voCtx,0,0,0, 0,0,0,255);
    VertexObjectBuilder_addVertex(voCtx,1,0,0, 255,0,0,255);
    VertexObjectBuilder_addVertex(voCtx,0,1,0, 0,255,0,255);
    VertexObjectBuilder_addVertex(voCtx,1,1,0, 255,255,0,255);
    
    VertexObjectBuilder_startObject(voCtx,GL_LINES);
    for(int r=0; r<rows; r++)
    {
        for(int c=0; c<cols; c++)
        {
            VertexObjectBuilder_addVertex(voCtx,xscale*c + halfXscale,0,0, 0,0,0,255);
            VertexObjectBuilder_addVertex(voCtx,xscale*c + halfXscale,1,0, 0,0,0,255);
        }
        VertexObjectBuilder_addVertex(voCtx,0,yscale*r + halfYscale,0, 0,0,0,255);
        VertexObjectBuilder_addVertex(voCtx,1,yscale*r + halfYscale,0, 0,0,0,255);
    }
}


void GenericRendering_draw()
{
    int voCount = VertexObjectBuilder_getVertexCount(voCtx);
    for(int o=0; o<voCount;o++)
    {
        int type;
        float* vertices;
        unsigned char* colors;
        int count;
        VertexObjectBuilder_getVertex(voCtx,o,&type,&vertices,&colors,&count);
        glVertexPointer(3, GL_FLOAT, 0, vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
        glEnableClientState(GL_COLOR_ARRAY);
        glDrawArrays(type, 0, count);            
    }
}
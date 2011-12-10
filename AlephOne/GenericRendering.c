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

struct VertexObjectBuilder* voCtxStatic;
struct VertexObjectBuilder* voCtxDynamic;

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
    
    voCtxDynamic = VertexObjectBuilder_init(malloc);
    
    voCtxStatic = VertexObjectBuilder_init(malloc);
    
    VertexObjectBuilder_startObject(voCtxStatic,GL_TRIANGLE_STRIP);
    
    VertexObjectBuilder_addVertex(voCtxStatic,0,0,0, 0,0,0,255);
    VertexObjectBuilder_addVertex(voCtxStatic,1,0,0, 255,0,0,255);
    VertexObjectBuilder_addVertex(voCtxStatic,0,1,0, 0,255,0,255);
    VertexObjectBuilder_addVertex(voCtxStatic,1,1,0, 255,255,0,255);
    
    VertexObjectBuilder_startObject(voCtxStatic,GL_LINES);
    for(int r=0; r<rows; r++)
    {
        for(int c=0; c<cols; c++)
        {
            VertexObjectBuilder_addVertex(voCtxStatic,xscale*c + halfXscale,0,0, 0,0,0,255);
            VertexObjectBuilder_addVertex(voCtxStatic,xscale*c + halfXscale,1,0, 0,0,0,255);
        }
        VertexObjectBuilder_addVertex(voCtxStatic,0,yscale*r + halfYscale,0, 0,0,0,255);
        VertexObjectBuilder_addVertex(voCtxStatic,1,yscale*r + halfYscale,0, 0,0,0,255);
    }
}

void GenericRendering_dynamic()
{
    float dx = 0.03;
    float dy = 0.03;
    VertexObjectBuilder_reset(voCtxDynamic);
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX, fInfo->fingerY - dy,0, 255,0,0,255);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX + dx, fInfo->fingerY + dy,0, 255,0,0,255);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->fingerX - dx, fInfo->fingerY + dy,0, 255,0,0,255);            
        }
    }
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    for(int f=0; f<16; f++)
    {
        struct FingerInfo* fInfo = PitchHandler_fingerState(f);
        if(fInfo->isActive)
        {
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX, fInfo->pitchY - dy,0, 0,255,0,255);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX + dx, fInfo->pitchY + dy,0, 0,255,0,255);
            VertexObjectBuilder_addVertex(voCtxDynamic,fInfo->pitchX - dx, fInfo->pitchY + dy,0, 0,255,0,255);            
        }
    }
    
    
    VertexObjectBuilder_startObject(voCtxDynamic, GL_TRIANGLES);
    float pitch=0;
    float x=0;
    float y=0;
    PitchHandler_getFretsBegin();
    int canAdd=1000;
    
    while(PitchHandler_getFret(&pitch, &x, &y) && canAdd)
    {
        canAdd *= VertexObjectBuilder_addVertex(voCtxDynamic,x, y - dy,0, 0,0,255,255);
        canAdd *= VertexObjectBuilder_addVertex(voCtxDynamic,x + dx, y + dy,0, 0,0,255,255);
        canAdd *= VertexObjectBuilder_addVertex(voCtxDynamic,x - dx, y + dy,0, 0,0,255,255);    
        canAdd--;
    }
     
}

void GenericRendering_drawVO(struct VertexObjectBuilder* vobj)
{
    int voCount = VertexObjectBuilder_getVertexCount(vobj);
    for(int o=0; o<voCount;o++)
    {
        struct VertexObject* vo = VertexObjectBuilder_getVertex(vobj,o);
        glVertexPointer(3, GL_FLOAT, 0, vo->vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, vo->colors);
        glEnableClientState(GL_COLOR_ARRAY);
        glDrawArrays(vo->type, 0, vo->count);            
    }
}

void GenericRendering_draw()
{
    GenericRendering_drawVO(voCtxStatic);
    GenericRendering_dynamic();
    GenericRendering_drawVO(voCtxDynamic);
}
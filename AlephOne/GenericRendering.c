//
//  GenericRendering.c
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "GenericRendering.h"
#include "PitchHandler.h"
//#include "Fretless.h"
#include "VertexObjectBuilder.h"
//#include "Transforms.h"
#include <OpenGLES/ES1/gl.h>

//This says that we should construct VertexObjectBuilder at a higher level
#include <stdlib.h>
#include <stdio.h>

#include "ObjectRendering.h"

struct Fretless_context;

void GenericRendering_drawVO(struct VertexObjectBuilder* vobj);

void GenericRendering_init(
                           struct PitchHandler_context* phctxArg, 
                           struct Fretless_context* fctxArg, 
                           void* imageContextArg,
                           void (*imageRenderArg)(void*,char*,unsigned int*,float*,float*,int),
                           void (*stringRenderArg)(void*,char*,unsigned int*,float*,float*,int)
                           )
{
    ObjectRendering_init(
                          VertexObjectBuilder_init(malloc,printf),
                          phctxArg,
                          fctxArg,
                          GL_TRIANGLES,
                          GL_TRIANGLE_STRIP,
                          GL_LINE_STRIP,
                          GL_LINES,
                          imageContextArg,
                          imageRenderArg,
                          stringRenderArg,
                          GenericRendering_drawVO
    );    
}


void GenericRendering_setup()
{
    ObjectRendering_loadImages();
}

void GenericRendering_updateLightOrientation(float x,float y, float z)
{
    ObjectRendering_updateLightOrientation(x,y,z);
}

void GenericRendering_camera()
{
    //glClearColor(0,0,0,255);
    glDisable(GL_LIGHTING);
    
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    //Transforms_getOrientation(scale);
    
    //glMultMatrixf(scale);
    glScalef(2,2,1);
    glTranslatef(-0.5,-0.5,0);    
    
    glClearColor(255,255,255,255);
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
            glDisable(GL_TEXTURE_2D);
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

            glTexCoordPointer(2,GL_FLOAT,0, vo->tex);  
            int texture = ObjectRendering_getTexture(vo->textureId);
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



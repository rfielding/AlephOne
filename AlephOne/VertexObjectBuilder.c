//
//  VertexObjectBuilder.c
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "VertexObjectBuilder.h"


#define VOVERTEXMAX 16384
#define VOOBJMAX 512

struct VertexObjectBuilder
{
    struct VertexObject vertexObjects[VOOBJMAX];
    int vertexObjectsCount;
    float gridVertices[VOVERTEXMAX];
    float gridTexCoord[VOVERTEXMAX];
    unsigned char gridColors[VOVERTEXMAX];
    float gridNormals[VOVERTEXMAX];
    int gridVerticesCount;  
    int (*fail)(const char*,...);
};


struct VertexObjectBuilder* VertexObjectBuilder_init(void* (*allocFn)(unsigned long),int (*fail)(const char*,...))
{
    struct VertexObjectBuilder* ctxp = allocFn(sizeof(struct VertexObjectBuilder));
    ctxp->fail = fail;
    VertexObjectBuilder_reset(ctxp);
    for(int i=0; i<VOVERTEXMAX; i++)
    {
        ctxp->gridVertices[i]    = (float)0xb0b0b0b0;
        ctxp->gridTexCoord[i]    = (float)0xb0b0b0b0;
        ctxp->gridNormals[i]     = (float)0xb0b0b0b0;
        ctxp->gridColors[i]      = (char)0xb0;
    }
    return ctxp;
}

void VertexObjectBuilder_reset(struct VertexObjectBuilder* ctxp)
{
    ctxp->gridVerticesCount=0;
    ctxp->vertexObjectsCount=0;
}

int VertexObjectBuilder_startObject(struct VertexObjectBuilder* ctxp,int type,int usingColors,int usesNormals,int usingTexture,int textureId)
{
    int obj = ctxp->vertexObjectsCount;
    int verts = ctxp->gridVerticesCount;
    if(obj >= VOOBJMAX)
    {
        ctxp->fail("trying to add too many objects!\n");
        return 0;
    }
    
    ctxp->vertexObjects[obj].count = 0;
    
    ctxp->vertexObjects[obj].vertices = &ctxp->gridVertices[3*verts];
    
    ctxp->vertexObjects[obj].colors = &ctxp->gridColors[4*verts];
    ctxp->vertexObjects[obj].usingColor = usingColors;
    
    ctxp->vertexObjects[obj].normals = &ctxp->gridNormals[3*verts];   
    ctxp->vertexObjects[obj].usesNormals = usesNormals;
    
    ctxp->vertexObjects[obj].tex = &ctxp->gridTexCoord[2*verts];   
    ctxp->vertexObjects[obj].usingTexture = usingTexture;
    ctxp->vertexObjects[obj].textureId = textureId;
    
    ctxp->vertexObjects[obj].type = type;
    
    ctxp->vertexObjectsCount++;
    return 1;
}

int VertexObjectBuilder_startColoredObject(struct VertexObjectBuilder* ctxp,int type)
{
    return VertexObjectBuilder_startObject(ctxp, type, 1, 0, 0,0);
}

int VertexObjectBuilder_startTexturedObject(struct VertexObjectBuilder* ctxp,int type,int textureId)
{
    return VertexObjectBuilder_startObject(ctxp, type, 0, 1, 1,textureId);
}

int VertexObjectBuilder_addColoredVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca)
{
    int obj = ctxp->vertexObjectsCount-1;
    int vert = ctxp->gridVerticesCount;
    if((4*ctxp->vertexObjects[obj].count + 4) >= VOVERTEXMAX)
    {
        ctxp->fail("adding too many textured vertices!\n");
        return 0;
    }
    ctxp->gridVertices[3*vert + 0] = x;
    ctxp->gridVertices[3*vert + 1] = y;
    ctxp->gridVertices[3*vert + 2] = z;
    ctxp->gridColors  [4*vert + 0] = cr;
    ctxp->gridColors  [4*vert + 1] = cg;
    ctxp->gridColors  [4*vert + 2] = cb;
    ctxp->gridColors  [4*vert + 3] = ca;
    ctxp->gridTexCoord[2*vert + 0] = 0;
    ctxp->gridTexCoord[2*vert + 1] = 0;
    ctxp->gridNormals [3*vert + 0] = 0;
    ctxp->gridNormals [3*vert + 1] = 0;
    ctxp->gridNormals [3*vert + 2] = 1;
    
    ctxp->gridVerticesCount++;        
    ctxp->vertexObjects[obj].count++;
    return 1;
}

int VertexObjectBuilder_addTexturedVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,float tx, float ty)
{
    int obj = ctxp->vertexObjectsCount-1;
    int vert = ctxp->gridVerticesCount;
    if(4*ctxp->vertexObjects[obj].count + 4 >= VOVERTEXMAX)
    {
        ctxp->fail("adding too many textured vertices!\n");
        return 0;
    }
    ctxp->gridVertices[3*vert + 0] = x;
    ctxp->gridVertices[3*vert + 1] = y;
    ctxp->gridVertices[3*vert + 2] = z;
    ctxp->gridColors  [4*vert + 0] = 0;
    ctxp->gridColors  [4*vert + 1] = 255;
    ctxp->gridColors  [4*vert + 2] = 0;
    ctxp->gridColors  [4*vert + 3] = 255;
    ctxp->gridTexCoord[2*vert + 0] = tx;
    ctxp->gridTexCoord[2*vert + 1] = ty;
    ctxp->gridNormals [3*vert + 0] = 0;
    ctxp->gridNormals [3*vert + 1] = 0;
    ctxp->gridNormals [3*vert + 2] = 1;
    
    ctxp->gridVerticesCount++;        
    ctxp->vertexObjects[obj].count++;
    return 1;
}

int VertexObjectBuilder_getVertexObjectsCount(struct VertexObjectBuilder* ctxp)
{
    return ctxp->vertexObjectsCount;
}

struct VertexObject* VertexObjectBuilder_getVertexObject(struct VertexObjectBuilder* ctxp,int idx)
{
    return &ctxp->vertexObjects[idx];
}


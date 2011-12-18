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
    unsigned char gridColors[VOVERTEXMAX];
    float gridNormals[VOVERTEXMAX];
    float gridTexCoord[VOVERTEXMAX];
    int gridVerticesCount;  
    int usingNormalsCount;
    int usingTextureCount;
    int usingColorCount;
    int (*fail)(const char*,...);
};


struct VertexObjectBuilder* VertexObjectBuilder_init(void* (*allocFn)(unsigned long),int (*fail)(const char*,...))
{
    struct VertexObjectBuilder* ctxp = allocFn(sizeof(struct VertexObjectBuilder));
    ctxp->fail = fail;
    VertexObjectBuilder_reset(ctxp);
    return ctxp;
}

void VertexObjectBuilder_reset(struct VertexObjectBuilder* ctxp)
{
    ctxp->gridVerticesCount=0;
    ctxp->vertexObjectsCount=0;
    ctxp->usingNormalsCount=0;
    ctxp->usingColorCount=0;
    ctxp->usingTextureCount=0;
}

int VertexObjectBuilder_startObject(struct VertexObjectBuilder* ctxp,int type,int usingColors,int usesNormals,int usingTexture,int textureId)
{
    if(ctxp->vertexObjectsCount >= VOOBJMAX)
    {
        ctxp->fail("trying to add too many objects!\n");
        return 0;
    }
    
    ctxp->vertexObjects[ctxp->vertexObjectsCount].count = 0;
    
    ctxp->vertexObjects[ctxp->vertexObjectsCount].vertices = &ctxp->gridVertices[3*ctxp->gridVerticesCount];
    if(usingColors)
    {
        ctxp->vertexObjects[ctxp->vertexObjectsCount].colors = &ctxp->gridColors[4*ctxp->gridVerticesCount];
        ctxp->vertexObjects[ctxp->vertexObjectsCount].usingColor = 1;
        ctxp->usingColorCount++;
    }
    else
    {
        ctxp->vertexObjects[ctxp->vertexObjectsCount].usingColor = 0;
        ctxp->vertexObjects[ctxp->vertexObjectsCount].colors = 0;
    }
    if(usesNormals)
    {
        ctxp->vertexObjects[ctxp->vertexObjectsCount].normals = &ctxp->gridNormals[3*ctxp->usingNormalsCount];   
        ctxp->vertexObjects[ctxp->vertexObjectsCount].usesNormals = 1;
        ctxp->usingNormalsCount++;
    }
    else
    {
        ctxp->vertexObjects[ctxp->vertexObjectsCount].normals = 0;
        ctxp->vertexObjects[ctxp->vertexObjectsCount].usesNormals = 0;
    }
    if(usingTexture)
    {
        ctxp->vertexObjects[ctxp->vertexObjectsCount].tex = &ctxp->gridTexCoord[2*ctxp->usingTextureCount];   
        ctxp->vertexObjects[ctxp->vertexObjectsCount].usingTexture = usingTexture;
        ctxp->vertexObjects[ctxp->vertexObjectsCount].textureId = textureId;
        ctxp->usingTextureCount++;
    }
    else
    {
        ctxp->vertexObjects[ctxp->vertexObjectsCount].tex = 0;
        ctxp->vertexObjects[ctxp->vertexObjectsCount].usingTexture = 0;
        ctxp->vertexObjects[ctxp->vertexObjectsCount].textureId = 0;
    }
    ctxp->vertexObjects[ctxp->vertexObjectsCount].type = type;
    ctxp->vertexObjectsCount++;
    return 1;
}

int VertexObjectBuilder_startColoredObject(struct VertexObjectBuilder* ctxp,int type)
{
    return VertexObjectBuilder_startObject(ctxp, type, 1, 0, 0,0);
}

int VertexObjectBuilder_startTexturedObject(struct VertexObjectBuilder* ctxp,int type,int textureId)
{
    return VertexObjectBuilder_startObject(ctxp, type, 0, 0, 1,textureId);
}

int VertexObjectBuilder_addColoredVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca)
{
    if(ctxp->vertexObjects[ctxp->vertexObjectsCount-1].count + 1 >= VOVERTEXMAX)
    {
        ctxp->fail("adding too many textured vertices!\n");
        return 0;
    }
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 0] = x;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 1] = y;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 2] = z;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 0] = cr;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 1] = cg;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 2] = cb;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 3] = ca;
    ctxp->gridVerticesCount++;        
    ctxp->vertexObjects[ctxp->vertexObjectsCount-1].count++;
    return 1;
}

int VertexObjectBuilder_addTexturedVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,float tx, float ty)
{
    if(ctxp->vertexObjects[ctxp->vertexObjectsCount-1].count + 1 >= VOVERTEXMAX)
    {
        ctxp->fail("adding too many textured vertices!\n");
        return 0;
    }
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 0] = x;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 1] = y;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 2] = z;
    if(ctxp->vertexObjects[ctxp->vertexObjectsCount-1].usingTexture)
    {
        ctxp->gridTexCoord[2*ctxp->usingTextureCount + 0] = tx;
        ctxp->gridTexCoord[2*ctxp->usingTextureCount + 1] = ty;
    }
    ctxp->gridVerticesCount++;        
    ctxp->vertexObjects[ctxp->vertexObjectsCount-1].count++;
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


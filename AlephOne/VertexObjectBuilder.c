//
//  VertexObjectBuilder.c
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "VertexObjectBuilder.h"


#define VOVERTEXMAX 8192
#define VOOBJMAX 512

struct VertexObjectBuilder
{
    struct VertexObject vertexObjects[VOOBJMAX];
    int vertexObjectsCount;
    float gridVertices[VOVERTEXMAX];
    unsigned char gridColors[VOVERTEXMAX];
    float gridNormals[VOVERTEXMAX];
    int gridVerticesCount;    
};


struct VertexObjectBuilder* VertexObjectBuilder_init(void* (*allocFn)(unsigned long))
{
    struct VertexObjectBuilder* ctxp = allocFn(sizeof(struct VertexObjectBuilder));
    VertexObjectBuilder_reset(ctxp);
    return ctxp;
}

void VertexObjectBuilder_reset(struct VertexObjectBuilder* ctxp)
{
    ctxp->gridVerticesCount=0;
    ctxp->vertexObjectsCount=0;
}

int VertexObjectBuilder_startObject(struct VertexObjectBuilder* ctxp,int type)
{
    if(ctxp->vertexObjectsCount+1 >= VOOBJMAX)return 0;
    ctxp->vertexObjects[ctxp->vertexObjectsCount].vertices = &ctxp->gridVertices[3*ctxp->gridVerticesCount];
    ctxp->vertexObjects[ctxp->vertexObjectsCount].colors = &ctxp->gridColors[4*ctxp->gridVerticesCount];
    ctxp->vertexObjects[ctxp->vertexObjectsCount].count = 0;
    ctxp->vertexObjects[ctxp->vertexObjectsCount].normals = &ctxp->gridNormals[3*ctxp->gridVerticesCount];
    ctxp->vertexObjects[ctxp->vertexObjectsCount].type = type;
    ctxp->vertexObjectsCount++;
    return 1;
}

int VertexObjectBuilder_addVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca, float nx, float ny, float nz)
{
    if(ctxp->vertexObjects[ctxp->vertexObjectsCount].count + 1 >= VOVERTEXMAX)return 0;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 0] = x;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 1] = y;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 2] = z;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 0] = cr;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 1] = cg;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 2] = cb;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 3] = ca;
    ctxp->gridNormals[3*ctxp->gridVerticesCount + 0] = nx;
    ctxp->gridNormals[3*ctxp->gridVerticesCount + 1] = ny;
    ctxp->gridNormals[3*ctxp->gridVerticesCount + 2] = nz;
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


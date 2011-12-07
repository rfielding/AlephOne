//
//  VertexObjectBuilder.c
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "VertexObjectBuilder.h"




struct VertexObjectBuilder
{
    struct VertexObject vertexObjects[256];
    int vertexObjectsCount;
    float gridVertices[4096];
    unsigned char gridColors[4096];
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

void VertexObjectBuilder_startObject(struct VertexObjectBuilder* ctxp,int type)
{
    ctxp->vertexObjects[ctxp->vertexObjectsCount].vertices = &ctxp->gridVertices[3*ctxp->gridVerticesCount];
    ctxp->vertexObjects[ctxp->vertexObjectsCount].colors = &ctxp->gridColors[4*ctxp->gridVerticesCount];
    ctxp->vertexObjects[ctxp->vertexObjectsCount].count = 0;
    ctxp->vertexObjects[ctxp->vertexObjectsCount].type = type;
    ctxp->vertexObjectsCount++;
}

void VertexObjectBuilder_addVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca)
{
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 0] = x;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 1] = y;
    ctxp->gridVertices[3*ctxp->gridVerticesCount + 2] = z;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 0] = cr;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 1] = cg;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 2] = cb;
    ctxp->gridColors[4*ctxp->gridVerticesCount + 3] = ca;
    ctxp->gridVerticesCount++;        
    ctxp->vertexObjects[ctxp->vertexObjectsCount-1].count++;
}

int VertexObjectBuilder_getVertexCount(struct VertexObjectBuilder* ctxp)
{
    return ctxp->vertexObjectsCount;
}

struct VertexObject* VertexObjectBuilder_getVertex(struct VertexObjectBuilder* ctxp,int idx)
{
    return &ctxp->vertexObjects[idx];
}


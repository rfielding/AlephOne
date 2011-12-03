//
//  VertexObjectBuilder.c
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "VertexObjectBuilder.h"


struct VertexObject {
    float* vertices;
    unsigned char* colors;
    int count;
    int type;
};
static struct VertexObject vertexObjects[256];
static int vertexObjectsCount=0;
static float gridVertices[4096];
static unsigned char gridColors[4096];
static int gridVerticesCount=0;

void vertexReset()
{
    gridVerticesCount=0;
    vertexObjectsCount=0;
}

void vertexObjectStart(int type)
{
    vertexObjects[vertexObjectsCount].vertices = &gridVertices[3*gridVerticesCount];
    vertexObjects[vertexObjectsCount].colors = &gridColors[4*gridVerticesCount];
    vertexObjects[vertexObjectsCount].count = 0;
    vertexObjects[vertexObjectsCount].type = type;
    vertexObjectsCount++;
}

void vertexAdd(float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca)
{
    gridVertices[3*gridVerticesCount + 0] = x;
    gridVertices[3*gridVerticesCount + 1] = y;
    gridVertices[3*gridVerticesCount + 2] = z;
    gridColors[4*gridVerticesCount + 0] = cr;
    gridColors[4*gridVerticesCount + 1] = cg;
    gridColors[4*gridVerticesCount + 2] = cb;
    gridColors[4*gridVerticesCount + 3] = ca;
    gridVerticesCount++;        
    vertexObjects[vertexObjectsCount-1].count++;
}

int vertexObjectCount()
{
    return vertexObjectsCount;
}

void vertexObjectGet(int idx,int* type,float** vertices,unsigned char** colors,int* count)
{
    *type = vertexObjects[idx].type;
    *vertices = vertexObjects[idx].vertices;
    *colors = vertexObjects[idx].colors;
    *count = vertexObjects[idx].count;
}


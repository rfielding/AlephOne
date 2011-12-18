//
//  VertexObjectBuilder.h
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct VertexObjectBuilder;

struct VertexObject {
    float* vertices;
    float* normals;
    float* tex;
    unsigned char* colors;
    int count;
    int type;
    int usesNormals;
    int usingTexture;
    int usingColor;
    int textureId;
};

//Invoke this first to get a handle on a builder for making vertex arrays
struct VertexObjectBuilder* VertexObjectBuilder_init(void* (*allocFn)(unsigned long),int (*fail)(const char*,...));


//Completely empty out the builder, so that there are no objects and no vertices
void VertexObjectBuilder_reset(struct VertexObjectBuilder* ctxp);

//Start creating a vertex object with type GL_LINES, GL_TRIANGLES, etc.
int VertexObjectBuilder_startColoredObject(struct VertexObjectBuilder* ctxp,int type);
int VertexObjectBuilder_startTexturedObject(struct VertexObjectBuilder* ctxp,int type,int textureId);

int VertexObjectBuilder_addColoredVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca);
int VertexObjectBuilder_addTexturedVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,float tx,float ty);


//When iterating objects, here is how you get an object to draw it
struct VertexObject* VertexObjectBuilder_getVertexObject(struct VertexObjectBuilder* ctxp,int idx);
//This is how many objects we have
int VertexObjectBuilder_getVertexObjectsCount(struct VertexObjectBuilder* ctxp);

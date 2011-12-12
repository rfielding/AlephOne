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
    unsigned char* colors;
    int count;
    int type;
};

//Invoke this first to get a handle on a builder for making vertex arrays
struct VertexObjectBuilder* VertexObjectBuilder_init(void* (*allocFn)(unsigned long));


//Completely empty out the builder, so that there are no objects and no vertices
void VertexObjectBuilder_reset(struct VertexObjectBuilder* ctxp);

//Start creating a vertex object with type GL_LINES, GL_TRIANGLES, etc.
int VertexObjectBuilder_startObject(struct VertexObjectBuilder* ctxp,int type);

//Add a vertex to an object
int VertexObjectBuilder_addVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca);


//When iterating objects, here is how you get an object to draw it
struct VertexObject* VertexObjectBuilder_getVertexObject(struct VertexObjectBuilder* ctxp,int idx);
//This is how many objects we have
int VertexObjectBuilder_getVertexObjectsCount(struct VertexObjectBuilder* ctxp);

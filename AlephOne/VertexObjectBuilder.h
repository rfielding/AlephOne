//
//  VertexObjectBuilder.h
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Rob Fielding Software.
//

/**
   Use this to build vertices for use with OpenGL.  There are no actual OpenGL dependencies, so
   this can be easily unit tested.  Usage is like this:
 
     //Each vob is essentially an independent layer of drawn objects, like OpenGL1.1 display lists
     vob = VertexObjectBuilder_init(malloc,printf);
     ....
     //Flush everything out of this layer
     VertexObjectBuilder_reset(vob);
     ....
     //Begin creating a new object
     VertexObjectBuilder_startColoredObject(vob,GL_TRIANGLES);
     ....
     VertexObjectBuilder_addColoredVertex(vob, ....);
 
   Using the API is a matter of doing a reset on every frame, a startObject before each
   new primitive type, and adding vertices.  Then the data that gets generated is easily
   drawn in the caller's OpenGL code by looking at the types, iterating, and drawing in order.
 
   This object is kind of like a layer, or a primitive for making scene graphs.
 */

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

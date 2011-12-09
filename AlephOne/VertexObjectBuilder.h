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

struct VertexObjectBuilder* VertexObjectBuilder_init(void* (*allocFn)(unsigned long));

void VertexObjectBuilder_reset(struct VertexObjectBuilder* ctxp);
int VertexObjectBuilder_startObject(struct VertexObjectBuilder* ctxp,int type);
int VertexObjectBuilder_addVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca);
struct VertexObject* VertexObjectBuilder_getVertex(struct VertexObjectBuilder* ctxp,int idx);
int VertexObjectBuilder_getVertexCount(struct VertexObjectBuilder* ctxp);

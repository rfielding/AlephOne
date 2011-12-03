//
//  VertexObjectBuilder.h
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct VertexObjectBuilder;

struct VertexObjectBuilder* VertexObjectBuilder_init(void* (*allocFn)(unsigned long));

void VertexObjectBuilder_reset(struct VertexObjectBuilder* ctxp);
void VertexObjectBuilder_startObject(struct VertexObjectBuilder* ctxp,int type);
void VertexObjectBuilder_addVertex(struct VertexObjectBuilder* ctxp,float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca);
void VertexObjectBuilder_getVertex(struct VertexObjectBuilder* ctxp,int idx,int* type,float** vertices,unsigned char** colors,int* count);
int VertexObjectBuilder_getVertexCount(struct VertexObjectBuilder* ctxp);

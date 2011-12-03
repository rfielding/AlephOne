//
//  VertexObjectBuilder.h
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


void vertexReset();
void vertexObjectStart(int type);
void vertexAdd(float x,float y,float z,unsigned char cr,unsigned char cg,unsigned char cb, unsigned char ca);
void vertexObjectGet(int idx,int* type,float** vertices,unsigned char** colors,int* count);
int vertexObjectCount();

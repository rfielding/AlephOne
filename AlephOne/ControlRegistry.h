//
//  ControlRegistry.h
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


void  ControlRegistry_AddFloat(
    void (*setValue)(void*,float),
    float (*getValue)(void*),
    char* (*getDescription)(void*,float),
    char* name,
    float minValue,
    float maxValue,
    int width,
    int height,
    void* ctx
);

int   ControlRegistry_Count();
char* ControlRegistry_Name(int idx);
int   ControlRegistry_Type(int idx);
char* ControlRegistry_GetDescription(int idx);

float ControlRegistry_GetFloat(int idx);
void  ControlRegistry_SetFloat(int idx, float val);
float ControlRegistry_GetFloatMin(int idx);
float ControlRegistry_GetFloatMax(int idx);

int   ControlRegistry_GetWidth(int idx);
int   ControlRegistry_GetHeight(int idx);

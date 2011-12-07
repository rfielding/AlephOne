//
//  ControlRegistry.c
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "ControlRegistry.h"

struct ControlRegistry_Control
{
    void (*setValue)(float);
    float (*getValue)();
    char* (*getDescription)(float);
    char* name;
    float minValue;
    float maxValue;
    int width;
    int height;    
} _registry[100];
static int _registrySize = 0;

void  ControlRegistry_AddFloat(
                               void (*setValue)(float),
                               float (*getValue)(),
                               char* (*getDescription)(float),
                               char* name,
                               float minValue,
                               float maxValue,
                               int width,
                               int height
                               )
{
    _registry[_registrySize].setValue = setValue;
    _registry[_registrySize].getValue = getValue;
    _registry[_registrySize].getDescription = getDescription;
    _registry[_registrySize].name = name;
    _registry[_registrySize].minValue = minValue;
    _registry[_registrySize].maxValue = maxValue;
    _registry[_registrySize].width = width;
    _registry[_registrySize].height = height;    
    _registrySize++;
}

int   ControlRegistry_Count()
{
    return _registrySize;
}

char* ControlRegistry_Name(int idx)
{
    return _registry[idx].name;
}

int   ControlRegistry_Type(int idx)
{
    return 0;    
}

char* ControlRegistry_GetDescription(int idx)
{
    return _registry[idx].getDescription(_registry[idx].getValue());
}

float ControlRegistry_GetFloat(int idx)
{
    return _registry[idx].getValue();
}

void  ControlRegistry_SetFloat(int idx, float val)
{
    _registry[idx].setValue(val);
}

float ControlRegistry_GetFloatMin(int idx)
{
    return _registry[idx].minValue;
}

float ControlRegistry_GetFloatMax(int idx)
{
    return _registry[idx].maxValue;
}

int   ControlRegistry_GetWidth(int idx)
{
    return _registry[idx].width;
}

int   ControlRegistry_GetHeight(int idx)
{
    return _registry[idx].height;
}
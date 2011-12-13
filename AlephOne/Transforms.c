//
//  Transforms.c
//  AlephOne
//
//  Created by Robert Fielding on 12/12/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "Transforms.h"



static float coordinateMatrix[16] = 
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

static float rot90Matrix[16] =
{
    0.0f,-1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f    
};

static float xflipMatrix[16] =
{
    -1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f    
};

static float scratchMatrix[16] = 
{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

void Transforms_getOrientation(float* matrix)
{
    for(int i=0; i<16; i++)
    {
        matrix[i] = coordinateMatrix[i];
    }
}

void Transforms_mult(float* matrix)
{
    for(int r=0; r<4; r++)
    {
        for(int c=0; c<4; c++)
        {
            scratchMatrix[4*r + c] = 0;
        }
    }
    for(int r=0; r<4; r++)
    {
        for(int c=0; c<4; c++)
        {
            for(int n=0; n<4; n++)
            {
                scratchMatrix[4*r + c] += 
                coordinateMatrix[4*n + c] * matrix[4*r + n];                
            }
        }
    }
    for(int i=0; i<16; i++)
    {
        coordinateMatrix[i] = scratchMatrix[i];
    }
}

void Transforms_clockwiseOrientation()
{
    Transforms_mult(rot90Matrix);
}

void Transforms_xflipOrientation()
{
    Transforms_mult(xflipMatrix);
}

void Transforms_translate(float* xp,float* yp)
{
    float xs = (*xp * 2) - 1;
    float ys = (*yp * 2) - 1;
    float xr = xs;
    float yr = ys;
    
    xr = 
    coordinateMatrix[4*0 + 0] * xs +
    coordinateMatrix[4*0 + 1] * ys;
    yr = 
    coordinateMatrix[4*1 + 0] * xs +
    coordinateMatrix[4*1 + 1] * ys;
    
    *xp = ( xr+1)/2;
    *yp = ( yr+1)/2;
}

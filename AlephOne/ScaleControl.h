//
//  ScaleControl.h
//  AlephOne
//
//  Created by Robert Fielding on 1/5/12.
//  Copyright (c) 2012 Check Point Software. All rights reserved.
//

struct VertexObjectBuilder;
struct Fretless_context;
struct WidgetTree_rect;

struct ScaleControl_data
{
    struct WidgetTree_rect* rect;
};

void ScaleControl_touchesInit(
                              int trianglesArg,
                              int trianglestripArg,
                              int linestripArg,
                              int linesArg,
                              struct VertexObjectBuilder* voCtxDynamicArg, 
                              struct Fretless_context* fctxArg
                              );

struct ScaleControl_data* ScaleControl_create(float x1,float y1, float x2,float y2);

void ScaleControl_clear(void* ctx);
void ScaleControl_toggle(void* ctx);
void ScaleControl_commit(void* ctx);
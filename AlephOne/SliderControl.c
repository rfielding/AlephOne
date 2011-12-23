//
//  SliderControl.c
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#include "SliderControl.h"
#include "VertexObjectBuilder.h"
#include "WidgetTree.h"
#include <stdlib.h>


static int triangles;
static int trianglestrip;
static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;


void SliderControl_init(
                      struct VertexObjectBuilder* voCtxDynamicArg,
                      struct PitchHandler_context* phctxArg,
                      int trianglesArg,
                      int trianglestripArg
                      )
{
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    phctx = phctxArg;
    voCtxDynamic = voCtxDynamicArg;
}

struct Slider_data
{
    int widgetId;
    float val;
    void (*setter)(void* ctx,float val);
    float (*getter)(void* ctx);
};

void Slider_render(void* ctx)
{
    struct Slider_data* slider = (struct Slider_data*)ctx;
    
    if(slider)
    {
        struct WidgetTree_rect* w = WidgetTree_get(slider->widgetId);
        if(w)
        {            
            float xv = w->x1 + slider->val * (w->x2 - w->x1);
            VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1, w->y1, 0, 0,255,0,200);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1, w->y2, 0, 0,  0,0,127);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y1, 0, 0,255,0,127);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y2, 0, 0,  0,0,127);    
            
            VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y1, 0, 0,  0,0,127);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y2, 0, 0,  0,0,127);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2, w->y1, 0, 0,  0,0,127);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2, w->y2, 0, 0,  0,0,127);      
        }
    }
}

void Slider_up(void* ctx,int finger,void* touch)
{
    //struct Slider_data* slider = (struct Slider_data*)ctx;
    //struct WidgetTree_rect* w = WidgetTree_get(slider->widgetId);
}

void Slider_down(void* ctx,int finger,void* touch,int isMoving,float x,float y, float velocity, float area)
{
    struct Slider_data* slider = (struct Slider_data*)ctx;
    if(slider)
    {
        struct WidgetTree_rect* w = WidgetTree_get(slider->widgetId);
        if(w)
        {
            float fx = (x - w->x1)/(w->x2 - w->x1);
            if(fx < 0)
            {
                fx = 0;
            }
            if(fx > 1)
            {
                fx = 1;
            }
            if(slider->setter)
            {
                slider->setter(ctx,fx);        
            }                    
            slider->val = fx;
        }
    }
}

struct WidgetTree_rect* CreateSlider(
                                     int widgetId,
                                     float x1,float y1,float x2,float y2,
                                     void (*setter)(void* ctx,float val), 
                                     float (*getter)(void* ctx),
                                     void (*render)(void* ctx)
                                     )
{
    struct WidgetTree_rect* widget = WidgetTree_add(widgetId, x1,y1,x2,y2);
    struct Slider_data* slider = malloc(sizeof(struct Slider_data));
    slider->widgetId = widgetId;
    slider->setter = setter;
    slider->getter = getter;
    if(slider->getter)
    {
        slider->val = slider->getter(slider);
    }
    else
    {
        slider->val = 0.5;
    }
    widget->ctx = slider;
    widget->render = render;
    widget->up = Slider_up;
    widget->down = Slider_down;
    return widget;
}


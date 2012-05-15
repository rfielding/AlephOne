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
static int linestrip;
static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;


void SliderControl_init(
                      struct VertexObjectBuilder* voCtxDynamicArg,
                      struct PitchHandler_context* phctxArg,
                      int trianglesArg,
                      int trianglestripArg,
                      int linestripArg
                      )
{
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    linestrip = linestripArg;
    phctx = phctxArg;
    voCtxDynamic = voCtxDynamicArg;
}



void Slider_render(void* ctx)
{
    struct Slider_data* slider = (struct Slider_data*)ctx;
    float dy = 0.002;
    float dx = 0.002;
    struct WidgetTree_rect* w = slider->rect;
    float xv = w->x1 + slider->getter(slider) * (w->x2 - w->x1);
    VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y1+dy, 0, 0,255,0,200);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y2-dy, 0, 0,  0,0,180);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y1+dy, 0, 0,255,0,180);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y2-dy, 0, 0,  0,0,180);    
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y1+dy, 0, 0, 50,50,150);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, xv,    w->y2-dy, 0, 0, 50,50,150);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y1+dy, 0, 0, 50,50,150);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y2-dy, 0, 0, 50,50,150); 
    
    VertexObjectBuilder_startColoredObject(voCtxDynamic,linestrip);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx,    w->y1+dy, 0, 0, 200, 0,150);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx,    w->y2-dy, 0, 0, 100,50, 50);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx,    w->y2-dy, 0, 0, 100,50, 80);
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx,    w->y1+dy, 0, 0, 200,50,150);             
    VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx,    w->y1+dy, 0, 0, 200,50,150);             
    float s = 0.02;
    dx = 0.4;
    dy = 0.06;
    VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,slider->label);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1, 0, 0,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1+dy, 0, 0,1);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1, 0, 1,0);
    VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1+dy, 0, 1,1);        
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
        struct WidgetTree_rect* w = slider->rect;
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
            slider->setter(ctx,fx);        
        }
    }
}


void GenericSetter(void* ctx, float val)
{
    struct Slider_data* slider = (struct Slider_data*)ctx;
    slider->val = val;
}

float GenericGetter(void* ctx)
{
    struct Slider_data* slider = (struct Slider_data*)ctx;
    return slider->val;
}

struct Slider_data* CreateSlider(
                                     unsigned int label,
                                     float x1,float y1,float x2,float y2,
                                     void (*setter)(void* ctx,float val), 
                                     float (*getter)(void* ctx)
                                     )
{
    struct WidgetTree_rect* widget = WidgetTree_add(x1,y1,x2,y2);
    struct Slider_data* slider = malloc(sizeof(struct Slider_data));
    slider->rect = widget;
    slider->label = label;
    //This is only used to give generic sliders some state that generic getters retrieve
    slider->val = 0.5;
    slider->setter = setter ? setter : GenericSetter;
    slider->getter = getter ? getter : GenericGetter;
    slider->val = slider->getter(slider);
    //Don't use val directly, as it's just here for the getter to have some state if the getter needs it
    widget->ctx = slider;
    widget->render = Slider_render;
    widget->up = Slider_up;
    widget->down = Slider_down;
    return slider;
}


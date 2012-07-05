//
//  ButtonControl.c
//  AlephOne
//
//  Created by Robert Fielding on 12/26/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


#include "ButtonControl.h"
#include "VertexObjectBuilder.h"
#include "WidgetTree.h"
#include <stdlib.h>
#include "WidgetConstants.h"
#include "GraphicsCommon.h"

static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;

void ButtonControl_init(
                        struct VertexObjectBuilder* voCtxDynamicArg,
                        struct PitchHandler_context* phctxArg
                        )
{
    phctx = phctxArg;
    voCtxDynamic = voCtxDynamicArg;
}



void Button_render(void* ctx)
{
    struct Button_data* button = (struct Button_data*)ctx;
    
    if(button)
    {
        float dy = 0.002;
        float dx = 0.002;
        struct WidgetTree_rect* w = button->rect;
        if(w)
        {            
            int r = 205 * ((button->val) %2) + 50;
            int g = 205 * ((button->val/2) %4) + 50;
            int b = 205 * ((button->val/4) %8) + 50;
            if(button->downState)
            {
                VertexObjectBuilder_startColoredObject(voCtxDynamic,GRAPHICS_TRIANGLE_STRIP);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y1+dy, 0, 255,255,255,150);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y2-dy, 0, 255,255,255,150);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y1+dy, 0, 255,255,255,150);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y2-dy, 0, 255,255,255,150);                            
            }
            VertexObjectBuilder_startColoredObject(voCtxDynamic,GRAPHICS_TRIANGLE_STRIP);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y1+dy, 0, r,  g,b,200);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y2-dy, 0, r,  g,b,180);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y1+dy, 0, r,  g,b,180);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y2-dy, 0, r,  g,b,180);   
            
            VertexObjectBuilder_startColoredObject(voCtxDynamic, GRAPHICS_LINES);
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y2-dy, 0, 255,255,255,180);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y2-dy, 0, 255,255,255,180);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y2-dy, 0, 255,255,255,180);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y1+dy, 0, 255,255,255,180);
            
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1+dx, w->y1+dy, 0, 255,255,255,150);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y1+dy, 0, 255,255,255,150);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y1+dy, 0, 255,255,255,150);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2-dx, w->y2-dy, 0, 255,255,255,150);
            
            float s = 0.01;
            dx = 0.4;
            dy = 0.06;
            VertexObjectBuilder_startTexturedObject(voCtxDynamic,GRAPHICS_TRIANGLE_STRIP,button->label);
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1, 0, 0,0);
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1+dy, 0, 0,1);
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1, 0, 1,0);
            VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1+dy, 0, 1,1);  
        }

    }
}

void Button_up(void* ctx,int finger,void* touch)
{
    struct Button_data* button = (struct Button_data*)ctx;
    if(button)
    {
        button->downState = 0;
    }
}

void Button_down(void* ctx,int finger,void* touch,int isMoving,float x,float y, float area)
{
    struct Button_data* button = (struct Button_data*)ctx;
    if(button)
    {
        struct WidgetTree_rect* w = button->rect;
        if(w && !isMoving)
        {
            //N-state toggle
            if(button->setter)
            {
                int newVal = (button->val + 1) % button->stateCount ;
                button->setter(button, newVal);
                button->val = newVal;
            }
        }
        button->downState = 1;
    }
}

struct Button_data* CreateButton(
                                     unsigned int label,
                                     float x1,float y1,float x2,float y2,
                                     void (*setter)(void* ctx,int val), 
                                     int (*getter)(void* ctx),
                                     int stateCount
                                     )
{
    struct WidgetTree_rect* widget = WidgetTree_add(x1,y1,x2,y2);
    struct Button_data* button = malloc(sizeof(struct Button_data));
    button->rect = widget;
    button->label = label;
    button->stateCount = stateCount;
    button->setter = setter;
    button->getter = getter;
    button->downState = 0;
    if(button->getter)
    {
        button->val = button->getter(button);
    }
    else
    {
        button->val = 0;
    }
    widget->ctx = button;
    widget->render = Button_render;
    widget->up = Button_up;
    widget->down = Button_down;
    return button;
}


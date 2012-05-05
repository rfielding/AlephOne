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


static int triangles;
static int trianglestrip;
static struct VertexObjectBuilder* voCtxDynamic;
static struct PitchHandler_context* phctx;

void ButtonControl_init(
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



void Button_render(void* ctx)
{
    struct Button_data* button = (struct Button_data*)ctx;
    
    if(button)
    {
        struct WidgetTree_rect* w = button->rect;
        if(w)
        {            
            int r = 255 * ((button->val) %2);
            int g = 255 * ((button->val/2) %4);
            int b = 255 * ((button->val/4) %8);
            if(button->downState)
            {
                VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1, w->y1, 0, 255,255,255,100);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1, w->y2, 0, 255,255,255,100);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2, w->y1, 0, 255,255,255,100);
                VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2, w->y2, 0, 255,255,255,100);                            
            }
            VertexObjectBuilder_startColoredObject(voCtxDynamic,trianglestrip);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1, w->y1, 0, r,  g,b,200);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x1, w->y2, 0, r,  g,b, 80);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2, w->y1, 0, r,  g,b,127);
            VertexObjectBuilder_addColoredVertex(voCtxDynamic, w->x2, w->y2, 0, r,  g,b, 80);                
        }

        float s = 0.01;
        float dx = 0.4;
        float dy = 0.06;
        VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,button->label);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1, 0, 0,0);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1+dy, 0, 0,1);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1, 0, 1,0);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1+dy, 0, 1,1);  
        /*
        s+=0.002;
        VertexObjectBuilder_startTexturedObject(voCtxDynamic,trianglestrip,button->label);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1, 0, 0,0);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+s, w->y1+dy, 0, 0,1);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1, 0, 1,0);
        VertexObjectBuilder_addTexturedVertex(voCtxDynamic, w->x1+dx+s, w->y1+dy, 0, 1,1);   
         */
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

void Button_down(void* ctx,int finger,void* touch,int isMoving,float x,float y, float velocity, float area)
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


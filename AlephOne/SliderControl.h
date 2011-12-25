//
//  SliderControl.h
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//


struct VertexObjectBuilder;
struct PitchHandler_context;

void SliderControl_init(
                        struct VertexObjectBuilder* voCtxDynamicArg,
                        struct PitchHandler_context* phctxArg,
                        int trianglesArg,
                        int trianglestripArg
                        );


struct WidgetTree_rect* CreateSlider(
                                     int widgetId,
                                     unsigned int label,
                                     float x1,float y1,float x2,float y2,
                                     void (*setter)(void* ctx,float val), 
                                     float (*getter)(void* ctx),
                                     void (*render)(void* ctx)
                                     );

void Slider_render(void* ctx);
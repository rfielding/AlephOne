//
//  ChannelOccupancyControl.h
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct VertexObjectBuilder;
struct Fretless_context;
struct WidgetTree_rect;

struct ChannelOccupancyControl_data
{
    struct WidgetTree_rect* rect;
};

void ChannelOccupancyControl_init(
                                         int trianglesArg,
                                         int trianglestripArg,
                                         int linestripArg,
                                         struct VertexObjectBuilder* voCtxDynamicArg, 
                                         struct Fretless_context* fctxArg
                                         );


struct ChannelOccupancyControl_data* ChannelOccupancyControl_create(float x1,float y1, float x2,float y2);

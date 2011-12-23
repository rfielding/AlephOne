//
//  ChannelOccupancyControl.h
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct VertexObjectBuilder;
struct Fretless_context;

void ChannelOccupancyControl_touchesInit(
                                         int trianglesArg,
                                         int trianglestripArg,
                                         int linestripArg,
                                         struct VertexObjectBuilder* voCtxDynamicArg, 
                                         struct Fretless_context* fctxArg
                                         );


void ChannelOccupancyControl_render(void* ctx);

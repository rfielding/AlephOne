//
//  SurfaceDrawHandling.h
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct VertexObjectBuilder;
struct PitchHandler_context;

void SurfaceDraw_init(
                      struct VertexObjectBuilder* voCtxDynamicArg,
                      struct PitchHandler_context* phctxArg,
                      int trianglesArg,
                      int trianglestripArg,
                      int linesArg
                      );

struct WidgetTree_rect* SurfaceDraw_create();

void SurfaceDraw_drawBackground();

void SurfaceDraw_render(void* ctx);
//
//  SurfaceDrawHandling.h
//  AlephOne
//
//  Created by Robert Fielding on 12/23/11.
//  Copyright 2011 Rob Fielding Software.
//

struct VertexObjectBuilder;
struct PitchHandler_context;

void SurfaceDraw_init(
                      struct VertexObjectBuilder* voCtxDynamicArg,
                      struct PitchHandler_context* phctxArg
                      );

struct WidgetTree_rect* SurfaceDraw_create();

void SurfaceDraw_drawBackground();

void SurfaceDraw_render(void* ctx);
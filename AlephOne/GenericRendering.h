//
//  GenericRendering.h
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//
struct PitchHandlerContext;

void GenericRendering_init(struct PitchHandlerContext* phctxArg);

//Done when OpenGL is initialized
void GenericRendering_setup();

//These are done on every frame
void GenericRendering_camera();
void GenericRendering_draw();

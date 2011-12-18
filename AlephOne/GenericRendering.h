//
//  GenericRendering.h
//  AlephOne
//
//  Created by Robert Fielding on 12/3/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

/*
   This code interacts directly with PitchHandler, which is almost like a model for this
   code which acts like a view (and UIView that invokes PitchHandler is a bit like the controller,
   in that it's the one responsible for instantiating and wiring objects together, forwarding touches,
   etc.).
 */

struct PitchHandler_context;
struct Fretless_context;
void GenericRendering_init(struct PitchHandler_context* phctxArg, struct Fretless_context* fctxArg);

void GenericRendering_updateLightOrientation(float x,float y, float z);
//Done when OpenGL is initialized
void GenericRendering_setup();

//These are done on every frame
void GenericRendering_camera();
void GenericRendering_draw();

//The OS invokes this to figure out what texture files it needs to load.
//I assume png, and that they are from 0 to n, and returns NULL when no more.
char* GenericRendering_getRequiredTexture(int idx);
void  GenericRendering_assignRequiredTexture(int idx,int val);

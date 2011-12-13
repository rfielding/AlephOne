//
//  GenericTouchHandling.h
//  AlephOne
//
//  Created by Robert Fielding on 12/2/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

struct PitchHandlerContext;

void GenericTouchHandling_touchesInit(struct PitchHandlerContext*);
void GenericTouchHandling_touchesUp(void* touch);
void GenericTouchHandling_touchesDown(void* touch,int isMoving,float x,float y);
void GenericTouchHandling_touchesFlush();
void GenericTouchHandling_tick();    

//Chorus is not a post-processing effect, but a voice duplication, so it's handled here
float GenericTouchHandling_getChorusLevel();
void GenericTouchHandling_setChorusLevel(float chorus);
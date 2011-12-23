//
//  WidgetTree.h
//  AlephOne
//
//  Created by Robert Fielding on 12/21/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#define ROOTWIDGET 0
#define NOWIDGET -1

struct WidgetTree_rect
{
    int identifier;
    
    float x1;
    float y1;
    float x2;
    float y2;
    
    void* ctx;
    void (*flush)(void* ctx);
    void (*up)(void* ctx,int finger,void* touch);
    void (*down)(void* ctx,int finger,void* touch,int isMoving,float x,float y, float velocity, float area);
    void (*tick)(void* ctx);
    void (*render)(void* ctx);
};

/**
 *  Clear out the widget tree.  Must be called before using this API
 */
void WidgetTree_clear();

/**
 *  The entire surface is bounded in ((0,0), (1,1)), with (0,0) as bottom corner
 *  
 *  Given a coordinate, tell us which widget was touched.
 *  The main surface is identifier 0.
 */
int WidgetTree_hitTest(float x,float y);



/**
 *  Put in a rectangle, (0,0) is bottom corner, (1,1) is upper right
 */
struct WidgetTree_rect* WidgetTree_add(int identifier, float x1, float y1, float x2, float y2);

/**
 *  Get a rectangle for an id
 */
struct WidgetTree_rect* WidgetTree_get(int identifier);

int WidgetTree_count();

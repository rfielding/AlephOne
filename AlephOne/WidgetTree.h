//
//  WidgetTree.h
//  AlephOne
//
//  Created by Robert Fielding on 12/21/11.
//  Copyright 2011 Rob Fielding Software.
//

#define ROOTWIDGET 0
#define NOWIDGET -1

struct WidgetTree_rect
{
    float x1;
    float y1;
    float x2;
    float y2;
    
    void* ctx;
    void (*flush)(void* ctx);
    void (*up)(void* ctx,int finger,void* touch);
    void (*down)(void* ctx,int finger,void* touch,int isMoving,float x,float y, float area);
    void (*tick)(void* ctx);
    void (*render)(void* ctx);
    
    int isActive;
};

void WidgetTree_init(    
    int (*failArg)(const char*,...),
    int (*loggerArg)(const char*,...)
);

/**
 *  The entire surface is bounded in ((0,0), (1,1)), with (0,0) as bottom corner
 *  
 *  Given a coordinate, tell us which widget was touched.
 *  The main surface is identifier 0.
 */
struct WidgetTree_rect* WidgetTree_hitTest(float x,float y);



/**
 *  Put in a rectangle, (0,0) is bottom corner, (1,1) is upper right
 *
 *  The root widget MUST be inserted first, as it's what we fall back on if hit tests miss.
 */
struct WidgetTree_rect* WidgetTree_add(float x1, float y1, float x2, float y2);

/**
 *  Get a rectangle (in order)
 */
struct WidgetTree_rect* WidgetTree_get(int order);

int WidgetTree_count();

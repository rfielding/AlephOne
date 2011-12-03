//
//  AlephOneViewController.m
//  AlephOne
//
//  Created by Robert Fielding on 10/14/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "AlephOneViewController.h"
#import "EAGLView.h"
#include "PitchHandler.h"

#import "VertexObjectBuilder.h"


void esgl1SetupCamera()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    static float scale[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
    PitchHandler_getOrientation(scale);
    
    glMultMatrixf(scale);
    glScalef(2,2,1);
    glTranslatef(-0.5,-0.5,0);        
}


void esgl1SetupGrid()
{
    int rows = PitchHandler_getRowCount();
    int cols = PitchHandler_getColCount();
              
    float xscale = 1.0/cols;
    float yscale = 1.0/rows;
    float halfXscale = 0.5*xscale;
    float halfYscale = 0.5*yscale;
     
    vertexReset();
    vertexObjectStart(GL_TRIANGLE_STRIP);
  
    vertexAdd(0,0,0, 0,0,0,255);
    vertexAdd(1,0,0, 255,0,0,255);
    vertexAdd(0,1,0, 0,255,0,255);
    vertexAdd(1,1,0, 255,255,0,255);
    
    vertexObjectStart(GL_LINES);
    for(int r=0; r<rows; r++)
    {
        for(int c=0; c<cols; c++)
        {
            vertexAdd(xscale*c + halfXscale,0,0, 0,0,0,255);
            vertexAdd(xscale*c + halfXscale,1,0, 0,0,0,255);
        }
        vertexAdd(0,yscale*r + halfYscale,0, 0,0,0,255);
        vertexAdd(1,yscale*r + halfYscale,0, 0,0,0,255);
    }
}


void vertexObjectsDraw()
{
    int voCount = vertexObjectCount();
    for(int o=0; o<voCount;o++)
    {
        int type;
        float* vertices;
        unsigned char* colors;
        int count;
        vertexObjectGet(o,&type,&vertices,&colors,&count);
        glVertexPointer(3, GL_FLOAT, 0, vertices);
        glEnableClientState(GL_VERTEX_ARRAY);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
        glEnableClientState(GL_COLOR_ARRAY);
        glDrawArrays(type, 0, count);            
    }
}

@interface AlephOneViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) CADisplayLink *displayLink;
@end

@implementation AlephOneViewController

@synthesize animating, context, displayLink;

- (void)awakeFromNib
{
    EAGLContext *aContext;// = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    //if (!aContext) {
        aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    //}
    
    if (!aContext)
        NSLog(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
        NSLog(@"Failed to set ES context current");
    
	self.context = aContext;
	[aContext release];
	
    [(EAGLView *)self.view setContext:context];
    [(EAGLView *)self.view setFramebuffer];
    
    //if ([context API] == kEAGLRenderingAPIOpenGLES2)
    //    [self loadShaders];
    
    animating = FALSE;
    animationFrameInterval = 1;
    self.displayLink = nil;
    
    esgl1SetupGrid();    
}

- (void)dealloc
{
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
    
    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
    
    [context release];
    
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (void)viewWillAppear:(BOOL)animated
{
    [self startAnimation];
    
    [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self stopAnimation];
    
    [super viewWillDisappear:animated];
}

- (void)viewDidUnload
{
	[super viewDidUnload];
	
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }

    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	self.context = nil;	
}

- (NSInteger)animationFrameInterval
{
    return animationFrameInterval;
}

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
    /*
	 Frame interval defines how many display frames must pass between each time the display link fires.
	 The display link will only fire 30 times a second when the frame internal is two on a display that refreshes 60 times a second. The default frame interval setting of one will fire 60 times a second when the display refreshes at 60 times a second. A frame interval setting of less than one results in undefined behavior.
	 */
    if (frameInterval >= 1) {
        animationFrameInterval = frameInterval;
        
        if (animating) {
            [self stopAnimation];
            [self startAnimation];
        }
    }
}

- (void)startAnimation
{
    if (!animating) {
        CADisplayLink *aDisplayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(drawFrame)];
        [aDisplayLink setFrameInterval:animationFrameInterval];
        [aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        self.displayLink = aDisplayLink;
        
        animating = TRUE;
    }
}

- (void)stopAnimation
{
    if (animating) {
        [self.displayLink invalidate];
        self.displayLink = nil;
        animating = FALSE;
    }
}



- (void)drawFrame
{
    [(EAGLView *)self.view setFramebuffer];
    [(EAGLView *)self.view tick];
        
    esgl1SetupCamera();
    vertexObjectsDraw();
    
    [(EAGLView *)self.view presentFramebuffer];
}



@end

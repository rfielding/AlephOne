//
//  EAGLView.m
//  OpenGLES_iPhone
//
//  Created by mmalc Crawford on 11/18/10.
//  Copyright 2010 Apple Inc. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "EAGLView.h"
#import <stdio.h>


#import "Fretless.h"
#import "PitchHandler.h"
#import "CoreMIDIRenderer.h"
#import "TouchMapping.h"

static struct Fretless_context* fretlessp = NULL;

void touchesInit()
{
    fretlessp = Fretless_init(CoreMIDIRenderer_midiPutch,CoreMIDIRenderer_midiFlush,malloc,CoreMIDIRenderer_midiFail,CoreMIDIRenderer_midiPassed,printf);
    CoreMIDIRenderer_midiInit(fretlessp);
    Fretless_boot(fretlessp);     
}

void touchesUp(void* touch)
{
    int finger  = TouchMapping_mapFinger(fretlessp, touch);
    if(finger < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger1");   
    }
    int finger2 = TouchMapping_mapFinger2(fretlessp, touch);
    if(finger < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger2");   
    }
    Fretless_up(fretlessp, finger);
    Fretless_up(fretlessp, finger2);
    TouchMapping_unmapFinger(fretlessp,touch);
    TouchMapping_unmapFinger2(fretlessp,touch);    
}

void touchesDown(
                 void* touch,
                 int isMoving,
                 float x,
                 float y
                 )
{
    int finger1;
    int finger2;
    float noteHi;
    float noteLo;
    int polygroup;
    float expr;
    finger1  = TouchMapping_mapFinger(fretlessp, touch);
    if(finger1 < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger1");   
    }    
    finger2  = TouchMapping_mapFinger2(fretlessp, touch);
    if(finger2 < 0)
    {
        CoreMIDIRenderer_midiFail("touch did not map to a finger2");   
    }    
    float noteRaw = PitchHandler_pickPitchRaw(
                                              finger1,
                                              x,
                                              y,
                                              &polygroup,
                                              &expr
                                              );
    float beginNote;
    float endNote;
    float note = PitchHandler_pickPitch(finger1,isMoving,noteRaw,&beginNote,&endNote);
    noteHi = note + (expr*expr)*0.2;
    noteLo = note - (expr*expr)*0.2;    
    if(isMoving)
    {
        Fretless_move(fretlessp,finger1,noteLo);
        Fretless_express(fretlessp, finger1, 0, expr);
        Fretless_move(fretlessp,finger2,noteHi);
        Fretless_express(fretlessp, finger2, 0, expr);        
    }
    else
    {
        float velocity = 1.0;
        int legato = 0;
        Fretless_down(fretlessp,finger1, noteLo,polygroup,velocity,legato); 
        Fretless_express(fretlessp, finger1, 0, expr);
        Fretless_down(fretlessp,finger2,noteHi,polygroup+8,velocity,legato); 
        Fretless_express(fretlessp, finger2, 0, expr);        
    }
}


@interface EAGLView (PrivateMethods)
- (void)createFramebuffer;
- (void)deleteFramebuffer;
@end

@implementation EAGLView

@synthesize context;

// You must implement this method
+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

//The EAGL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:.
- (id)initWithCoder:(NSCoder*)coder
{
    self = [super initWithCoder:coder];
	if (self) {
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                        nil];
    }
    [self setMultipleTouchEnabled:TRUE];
    touchesInit();
    return self;
}

- (void)dealloc
{
    [self deleteFramebuffer];    
    [context release];
    
    [super dealloc];
}

- (void)setContext:(EAGLContext *)newContext
{
    if (context != newContext) {
        [self deleteFramebuffer];
        
        [context release];
        context = [newContext retain];
        
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)createFramebuffer
{
    if (context && !defaultFramebuffer) {
        [EAGLContext setCurrentContext:context];
        
        // Create default framebuffer object.
        glGenFramebuffers(1, &defaultFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
        
        // Create color render buffer and allocate backing store.
        glGenRenderbuffers(1, &colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferHeight);
        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
}

- (void)deleteFramebuffer
{
    if (context) {
        [EAGLContext setCurrentContext:context];
        
        if (defaultFramebuffer) {
            glDeleteFramebuffers(1, &defaultFramebuffer);
            defaultFramebuffer = 0;
        }
        
        if (colorRenderbuffer) {
            glDeleteRenderbuffers(1, &colorRenderbuffer);
            colorRenderbuffer = 0;
        }
    }
}

- (void)setFramebuffer
{
    if (context) {
        [EAGLContext setCurrentContext:context];
        
        if (!defaultFramebuffer)
            [self createFramebuffer];
        
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
        
        glViewport(0, 0, framebufferWidth, framebufferHeight);
    }
}

- (BOOL)presentFramebuffer
{
    BOOL success = FALSE;
    
    if (context) {
        [EAGLContext setCurrentContext:context];
        
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        
        success = [context presentRenderbuffer:GL_RENDERBUFFER];
    }
    
    return success;
}

- (void)layoutSubviews
{
    // The framebuffer will be re-created at the beginning of the next setFramebuffer method call.
    [self deleteFramebuffer];
}

- (void)tick
{
    //    Fretless_tick(fretlessp);
}


- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    NSArray* touchArray = [touches allObjects];
    int touchCount = [touches count];
    for(int t=0; t < touchCount; t++)
    {
        UITouch* touch = [touchArray objectAtIndex:t];
        UITouchPhase phase = [touch phase];
        if(phase == UITouchPhaseBegan)
        {            
            touchesDown(
                touch,
                phase == UITouchPhaseMoved,
                [touch locationInView:self].x/framebufferWidth,
                [touch locationInView:self].y/framebufferHeight
            );
        }
    }
    Fretless_flush(fretlessp);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    NSArray* touchArray = [touches allObjects];
    int touchCount = [touches count];
    for(int t=0; t < touchCount; t++)
    {
        UITouch* touch = [touchArray objectAtIndex:t];
        UITouchPhase phase = [touch phase];
        if(phase == UITouchPhaseMoved)
        {
            touchesDown(
                touch,
                phase == UITouchPhaseMoved,
                [touch locationInView:self].x/framebufferWidth,
                [touch locationInView:self].y/framebufferHeight
            );
        }
    }
    Fretless_flush(fretlessp);
}



- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray* touchArray = [touches allObjects];
    int touchCount = [touchArray count];
    for(int t=0; t < touchCount; t++)
    {
        UITouch* touch = [touchArray objectAtIndex:t];
        UITouchPhase phase = [touch phase];
        if(phase==UITouchPhaseEnded)
        {
            touchesUp(touch);
        }
    }
    Fretless_flush(fretlessp);
}
    
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSArray* touchArray = [touches allObjects];
    int touchCount = [touchArray count];
    for(int t=0; t < touchCount; t++)
    {
        UITouch* touch = [touchArray objectAtIndex:t];
        UITouchPhase phase = [touch phase];
        if(phase==UITouchPhaseCancelled)
        {
            touchesUp(touch);
        }
    }
    Fretless_flush(fretlessp);
}

@end

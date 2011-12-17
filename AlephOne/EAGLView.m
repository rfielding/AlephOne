//
//  EAGLView.m
//  OpenGLES_iPhone
//
//  Created by mmalc Crawford on 11/18/10.
//  Copyright 2010 Apple Inc. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "EAGLView.h"
#import "GenericTouchHandling.h"
#import "PitchHandler.h"
#import "Fretless.h"
#import "Transforms.h"
#import "GenericRendering.h"
#import "PressureSensor.h"
#include "CoreMIDIRenderer.h"

static BOOL isInitialized = FALSE;
static struct PitchHandlerContext* phctx;
static struct Fretless_context* fctx;

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

- (void)configureSurface
{
    //0.0 is C
    PitchHandler_clearFrets(phctx);
    
    float baseNote = 2.0; //D
    //First tetrachord
    PitchHandler_placeFret(phctx,baseNote + 0.0,3);
    PitchHandler_placeFret(phctx,baseNote + 1.0,2);
    PitchHandler_placeFret(phctx,baseNote + 1.5,1);
    PitchHandler_placeFret(phctx,baseNote + 2.0,3);
    PitchHandler_placeFret(phctx,baseNote + 3.0,3);
    PitchHandler_placeFret(phctx,baseNote + 4.0,2);
    //Second tetrachord
    PitchHandler_placeFret(phctx,baseNote + 0.0 + 5,3);
    PitchHandler_placeFret(phctx,baseNote + 1.0 + 5,2);
    PitchHandler_placeFret(phctx,baseNote + 1.5 + 5,1);
    //Tetrachord from fifth
    PitchHandler_placeFret(phctx,baseNote + 0.0 + 7,3);
    PitchHandler_placeFret(phctx,baseNote + 1.0 + 7,2);
    
    PitchHandler_placeFret(phctx,baseNote + 1.5 + 7,1);
    PitchHandler_placeFret(phctx,baseNote + 2.0 + 7,3);
    PitchHandler_placeFret(phctx,baseNote + 3.0 + 7,3);
    PitchHandler_placeFret(phctx,baseNote + 4.0 + 7,2);
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        PitchHandler_setColCount(phctx,12);
        PitchHandler_setRowCount(phctx,6);                
    }
    else
    {
        PitchHandler_setColCount(phctx,5);
        PitchHandler_setRowCount(phctx,3);        
    }
    for(int s=0; s<16; s++)
    {
        //Set to Just fourths
        PitchHandler_setTuneInterval(phctx,s,4.9804499913461244);
    }
    PitchHandler_setNoteDiff(phctx,45); //A is bottom corner
    PitchHandler_setTuneSpeed(phctx,0.25);
    Fretless_setMidiHintChannelSpan(fctx, 16);
    Fretless_setMidiHintChannelBendSemis(fctx,2);
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
    if(isInitialized==FALSE)
    {        
        //PressureSensor_setup();
        Transforms_clockwiseOrientation();
        
        phctx = PitchHandler_init(malloc);
        fctx = Fretless_init(
            CoreMIDIRenderer_midiPutch,
            CoreMIDIRenderer_midiFlush,
            malloc,
            CoreMIDIRenderer_midiFail,
            CoreMIDIRenderer_midiPassed,
            printf
        );
        GenericRendering_init(phctx,fctx);
        GenericTouchHandling_touchesInit(phctx,fctx,CoreMIDIRenderer_midiFail,printf);
        CoreMIDIRenderer_midiInit(fctx);
        [self configureSurface];
        
        isInitialized=TRUE;
    }
    return self;
}

- (void)dealloc
{
    [self deleteFramebuffer];    
    [context release];
    
    [super dealloc];
}

- (void)setup:(EAGLContext*)newContext
{
    [self setContext:newContext];
    [self setFramebuffer];    
    
    GenericRendering_setup();    
    
}

- (void)drawFrame
{
    [self setFramebuffer];
    [self tick];
    
    GenericRendering_camera();
    GenericRendering_draw();
    
    [self presentFramebuffer];
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
    tickClock++;
    tickClock%=4;
    if(tickClock==0)
    {
        GenericRendering_updateLightOrientation(
            PressureSensor_xNorm,
            PressureSensor_yNorm,
            PressureSensor_zNorm
        );
        GenericTouchHandling_tick();                    
    }
}


- (void)handleTouchDown:(NSSet*)touches inPhase:(UITouchPhase)expectPhase
{
    NSArray* touchArray = [touches allObjects];
    int touchCount = [touches count];
    for(int t=0; t < touchCount; t++)
    {
        UITouch* touch = [touchArray objectAtIndex:t];
        UITouchPhase phase = [touch phase];
        if(phase == expectPhase)
        {            
            float x = [touch locationInView:self].x/framebufferWidth;
            float y = 1 - [touch locationInView:self].y/framebufferHeight;
            Transforms_translate(&x, &y);
            
            //Yes, the forbidden finger area touch is back! (For now anyways)
            float area = 1.0;
            id valFloat = [touch valueForKey:@"pathMajorRadius"];
            if(valFloat != nil)
            {
                area = ([valFloat floatValue]-4)/7.0;
                area *= area*area;
            }
            //NSLog(@"%f",PressureSensor_pressure);
            GenericTouchHandling_touchesDown(touch,phase == UITouchPhaseMoved,x,y, PressureSensor_pressure, area); 
        }
    }
    GenericTouchHandling_touchesFlush();    
}

- (void)handleTouchUp:(NSSet*)touches inPhase:(UITouchPhase)expectPhase
{
    NSArray* touchArray = [touches allObjects];
    int touchCount = [touchArray count];
    for(int t=0; t < touchCount; t++)
    {
        UITouch* touch = [touchArray objectAtIndex:t];
        UITouchPhase phase = [touch phase];
        if(phase==expectPhase)
        {
            GenericTouchHandling_touchesUp(touch);
        }
    }
    GenericTouchHandling_touchesFlush();    
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    [self handleTouchDown:touches inPhase:UITouchPhaseBegan];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    [self handleTouchDown:touches inPhase:UITouchPhaseMoved];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self handleTouchUp:touches inPhase:UITouchPhaseEnded];
}
    
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self handleTouchUp:touches inPhase:UITouchPhaseCancelled];
}

@end

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
#import "CoreMIDIRenderer.h"

static struct Fretless_context* fretlessp = NULL;

int Fretless_util_mapFinger2(struct Fretless_context* ctxp, void* touch)
{
    return Fretless_util_mapFinger(ctxp, (void*)((int)touch ^ 0xFFFFFFFF));
}

void Fretless_util_unmapFinger2(struct Fretless_context* ctxp, void* touch)
{
    Fretless_util_unmapFinger(ctxp, (void*)((int)touch ^ 0xFFFFFFFF));
}

//Quick oct rounding hack
float pickPitch(int finger,int isMoving,float x,float y,int* stringP,float* exprP)
{
    static int   lastFingerDown = -1;
    static float lastNoteDown = 0;
    static int   octDiff = 48;
    static int   octDiffOurs = -1;
    static int   octDiffByFinger[16];
    float tuning = 12*log2f(4.0/3); //Just fourths (rather than simply diatonic fourth)
    
    if( isMoving )
    {
        octDiffOurs = octDiffByFinger[finger];
    }
    else
    {
        lastFingerDown = finger;
        octDiffOurs = octDiff;
        octDiffByFinger[finger] = octDiff;
    }
    
    *stringP = (3.0 * x);
    *exprP = (3.0*x) - *stringP;
    float fret = (tuning * y) - 0.5;
    float thisPitch = (fret + (*stringP)*5 + octDiffOurs);  
    
    if(finger == lastFingerDown)
    {
        float diff = (thisPitch - lastNoteDown);
        if(diff > 6.5)
        {
            thisPitch -= 12;
            octDiff -= 12;
            octDiffOurs -= 12;
        }
        if(diff <= -6.5)
        {
            thisPitch += 12;
            octDiff += 12;
            octDiffOurs += 12;
        }
        while(thisPitch < -0.5)
        {
            thisPitch += 12;
            octDiff += 12;
            octDiffOurs += 12;
        }
        while(thisPitch >= 127.5)
        {
            thisPitch -= 12;
            octDiff -= 12;
            octDiffOurs -= 12;
        }
        lastNoteDown = thisPitch;
    }
    octDiffByFinger[finger] = octDiffOurs;        
    return thisPitch;
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
    fretlessp = Fretless_init(midiPutch,midiFlush,malloc,midiFail,midiPassed,printf);
    Fretless_setMidiHintChannelBendSemis(fretlessp, 2);
    Fretless_setMidiHintChannelSpan(fretlessp, 4);
    midiInit(fretlessp);
    Fretless_boot(fretlessp);    
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

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    NSArray* touchArray = [touches allObjects];
    int touchCount = [touches count];
    for(int t=0; t < touchCount; t++)
    {
        UITouch* touch = [touchArray objectAtIndex:t];
        UITouchPhase phase = [touch phase];
        if(phase == UITouchPhaseBegan)
        {
            int finger  = Fretless_util_mapFinger(fretlessp, touch);
            int finger2 = Fretless_util_mapFinger2(fretlessp, touch);
            int string;
            float expr;
            float note = pickPitch(
                                   finger, 0,
                                   [touch locationInView:self].x/framebufferWidth,
                                   [touch locationInView:self].y/framebufferHeight,
                                   &string,
                                   &expr
                                   );
            int polygroup = string;
            float velocity = 1.0;
            int legato = 0;
            float noteHi = note + (expr*expr)*0.25;
            float noteLo = note - (expr*expr)*0.25;
            Fretless_down(fretlessp,finger, noteLo,polygroup,velocity,legato); 
            Fretless_down(fretlessp,finger2,noteHi,polygroup+8,velocity,legato); 
        }
    }
    Fretless_flush(fretlessp);
}

- (void)tick
{
//    Fretless_tick(fretlessp);
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
            int finger  = Fretless_util_mapFinger(fretlessp, touch);
            int finger2 = Fretless_util_mapFinger2(fretlessp, touch);
            int string;
            float expr;
            float note = pickPitch(finger, 1,
                                   [touch locationInView:self].x/framebufferWidth,
                                   [touch locationInView:self].y/framebufferHeight,
                                   &string,
                                   &expr
                                   );
            float noteHi = note + (expr*expr)*0.25;
            float noteLo = note - (expr*expr)*0.25;
            Fretless_move(fretlessp,finger,noteLo);
            Fretless_move(fretlessp,finger2,noteHi);
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
            int finger  = Fretless_util_mapFinger(fretlessp, touch);
            int finger2 = Fretless_util_mapFinger2(fretlessp, touch);
            Fretless_up(fretlessp, finger);
            Fretless_up(fretlessp, finger2);
            Fretless_util_unmapFinger(fretlessp,touch);
            Fretless_util_unmapFinger2(fretlessp,touch);
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
            int finger  = Fretless_util_mapFinger(fretlessp, touch);
            int finger2 = Fretless_util_mapFinger2(fretlessp, touch);
            Fretless_up(fretlessp, finger);
            Fretless_up(fretlessp, finger2);
            Fretless_util_unmapFinger(fretlessp,touch);
            Fretless_util_unmapFinger2(fretlessp,touch);
        }
    }
    Fretless_flush(fretlessp);
}

@end

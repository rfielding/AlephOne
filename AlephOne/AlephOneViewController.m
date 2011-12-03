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

// Uniform index.
enum {
    UNIFORM_TRANSLATE,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

// Attribute index.
enum {
    ATTRIB_VERTEX,
    ATTRIB_COLOR,
    NUM_ATTRIBUTES
};

static GLfloat gridVertices[1024];
static GLfloat gridColors[1024];
static int gridVerticesCount=0;

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

void esgl1DrawBackground()
{
    static const GLfloat squareVertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f,  1.0f,
        1.0f,  1.0f,
    };
    
    static const GLubyte squareColors[] = {
        0, 0,   0, 255,
        255,   0, 0, 255,
        0,     255,   0,   255,
        0,   0, 255, 255,
    };    
    glVertexPointer(2, GL_FLOAT, 0, squareVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, squareColors);
    glEnableClientState(GL_COLOR_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void esgl1SetupGrid()
{
    int rows = PitchHandler_getRowCount();
    int cols = PitchHandler_getColCount();
        
    static const GLfloat gridRawVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.5f, 0.2f, 0.3f,
        0.0f, 0.5f, 0.0f,
        0.5f, 0.8f, 0.3f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.5f, 0.8f, 0.3f,
        1.0f, 0.5f, 0.0f,
        0.5f, 0.2f, 0.3f,
        1.0f, 0.0f, 0.0f,
        
        1.0f, 0.0f, 0.0f,
        1.5f, 0.2f, 0.3f,
        1.0f, 0.5f, 0.0f,
        1.5f, 0.8f, 0.3f,
        1.0f, 1.0f, 0.0f,
        2.0f, 1.0f, 0.0f,
        1.5f, 0.8f, 0.3f,
        2.0f, 0.5f, 0.0f,
        1.5f, 0.2f, 0.3f,
        2.0f, 0.0f, 0.0f
    };
    static const GLfloat gridRawColors[] = {
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 0.0f
    };
      
    float xscale = 0.5/cols;
    float yscale = 0.5/rows;
    float zscale = 0.5;
    gridVerticesCount = 0;    

//    for(int r=0; r<rows; r++)
    {
//        for(int c=0; c<cols; c++)
        int c=0;
        {
            for(int g=0; g<10; g++)
            {
                gridVertices[gridVerticesCount + 0] = gridRawVertices[3*g + 0] * xscale + c*xscale;
                gridVertices[gridVerticesCount + 1] = gridRawVertices[3*g + 1] * yscale + c*yscale;
                gridVertices[gridVerticesCount + 2] = gridRawVertices[3*g + 2] * zscale;
                gridColors[gridVerticesCount + 0] = gridRawColors[3*g + 0];
                gridColors[gridVerticesCount + 1] = gridRawColors[3*g + 1];
                gridColors[gridVerticesCount + 2] = gridRawColors[3*g + 2];
                gridVerticesCount+=3;
            }
        }
    }
}

void esgl1DrawGrid()
{
    glVertexPointer(3, GL_FLOAT, 0, gridVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, gridColors);
    glEnableClientState(GL_COLOR_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, gridVerticesCount);    
}



@interface AlephOneViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) CADisplayLink *displayLink;
- (BOOL)loadShaders;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;
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
    esgl1DrawBackground();

    [(EAGLView *)self.view presentFramebuffer];
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source)
    {
        NSLog(@"Failed to load vertex shader");
        return FALSE;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
        glDeleteShader(*shader);
        return FALSE;
    }
    
    return TRUE;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog(@"Failed to compile vertex shader");
        return FALSE;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog(@"Failed to compile fragment shader");
        return FALSE;
    }
    
    // Attach vertex shader to program.
    glAttachShader(program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(program, ATTRIB_VERTEX, "position");
    glBindAttribLocation(program, ATTRIB_COLOR, "color");
    
    // Link program.
    if (![self linkProgram:program])
    {
        NSLog(@"Failed to link program: %d", program);
        
        if (vertShader)
        {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader)
        {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (program)
        {
            glDeleteProgram(program);
            program = 0;
        }
        
        return FALSE;
    }
    
    // Get uniform locations.
    uniforms[UNIFORM_TRANSLATE] = glGetUniformLocation(program, "translate");
    
    // Release vertex and fragment shaders.
    if (vertShader)
        glDeleteShader(vertShader);
    if (fragShader)
        glDeleteShader(fragShader);
    
    return TRUE;
}

@end

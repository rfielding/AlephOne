//
//  ObjectRendering.c
//  AlephOne
//
//  Created by Robert Fielding on 12/18/11.
//  Copyright 2011 Check Point Software. All rights reserved.
//
/**
   This place is a bit of a mosh pit of dependencies, because it wires together a lot of
   functions.
 */

#include "VertexObjectBuilder.h"
#include "ObjectRendering.h"
#include "Fretless.h"
#include "Fret.h"
#include "PitchHandler.h"
#include "WidgetTree.h"

#include "SurfaceTouchHandling.h"
#include "SurfaceDrawHandling.h"

#include "FretlessCommon.h"
#include "Parameters.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "WidgetConstants.h"
#include "ChannelOccupancyControl.h"
#include "ScaleControl.h"
#include "SliderControl.h"
#include "ButtonControl.h"
#include "RawEngine.h"

#define PAGEMAX 8

static void* ObjectRendering_imageContext;
static void (*ObjectRendering_imageRender)(void*,char*,unsigned int*,float*,float*,int);
static void (*ObjectRendering_stringRender)(void*,char*,unsigned int*,float*,float*,int);
static void (*ObjectRendering_drawVO)();

static int triangles;
static int trianglestrip;
static int linestrip;

static struct VertexObjectBuilder* voCtxDynamic;
//static struct VertexObjectBuilder* voCtxStatic;
static struct PitchHandler_context* phctx;
static struct Fret_context* fretctx;
static struct Fretless_context* fctx;


static char* requiredTexture[] = {
    "alephonep2",
    "ashmedi"
};

#define IMAGECOUNT 2


#define TEXTURECOUNT 256
static unsigned int textures[TEXTURECOUNT];
static float textureWidth[TEXTURECOUNT];
static float textureHeight[TEXTURECOUNT];
static float lightPosition[3];

struct Button_data* pagePrevButton;
struct Button_data* pageNextButton;

//All the references to controls
struct Slider_data* baseSlider;
struct Slider_data* widthSlider;
struct Slider_data* heightSlider;

struct Slider_data* intonationSlider;
struct Slider_data* rootNoteSlider;


struct Slider_data* midiChannelSlider;
struct Slider_data* midiChannelSpanSlider;
struct Slider_data* midiBendSlider;

struct Button_data* legatoButton;
struct Button_data* polyButton;
struct Slider_data* baseVolumeSlider;

struct Button_data* octAutoButton;

struct Button_data* scaleControlButton;
struct ScaleControl_data* scaleControl;
struct Button_data* scaleClearButton;
struct Button_data* scaleToggleButton;
struct Button_data* scaleCommitButton;

struct Button_data* initialSnapButton;
struct Slider_data* snapSpeedSlider;

struct Button_data* engineButton;

struct Slider_data* distortionSlider;
struct Slider_data* detuneSlider;
struct Slider_data* timbreSlider;
struct Slider_data* reverbSlider;
struct Slider_data* sensitivitySlider;

static char stringRenderBuffer[1024];

static float baseNote = 2.0;
static int currentPage = 0;

void WidgetsAssemble();

void ObjectRendering_updateLightOrientation(float x,float y, float z)
{
    lightPosition[0] = x;
    lightPosition[1] = y;
    lightPosition[2] = z;
}

//Pass a funtion pointer to this for anything that needs to update a string
//The impl might need to maintain more state to be efficient.
void reRenderString(char* val,unsigned int texture)
{
    //These need to be re-rendered into the same slot
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 val,
                                 &textures[texture],
                                 &textureWidth[texture],
                                 &textureHeight[texture],
                                 1
                                 );    
    //    *w = textureWidth[texture];
    //    *h = textureHeight[texture];
}

void renderLabel(char* label, unsigned int texture)
{
    ObjectRendering_stringRender(
                                 ObjectRendering_imageContext,
                                 label,
                                 &textures[texture],
                                 &textureWidth[texture],
                                 &textureHeight[texture],
                                 0
                                 );    
}

void ObjectRendering_init(
                           struct VertexObjectBuilder* voCtxDynamicArg,
                           //struct VertexObjectBuilder* voCtxStaticArg,
                           struct PitchHandler_context* phctxArg,
                           struct Fretless_context* fctxArg,
                           int trianglesArg,
                           int trianglestripArg,
                           int linestripArg,
                           int linesArg,
                           void* ObjectRendering_imageContextArg,
                           void (*ObjectRendering_imageRenderArg)(void*,char*,unsigned int*,float*,float*,int),
                           void (*ObjectRendering_stringRenderArg)(void*,char*,unsigned int*,float*,float*,int),
                           void (*ObjectRendering_drawVOArg)()
                           )
{
    voCtxDynamic = voCtxDynamicArg;
    //voCtxStatic = voCtxStaticArg;
    phctx = phctxArg;
    fctx = fctxArg;
    fretctx = PitchHandler_frets(phctx);
    triangles = trianglesArg;
    trianglestrip = trianglestripArg;
    linestrip = linestripArg;
    ObjectRendering_imageContext = ObjectRendering_imageContextArg;
    ObjectRendering_imageRender = ObjectRendering_imageRenderArg;
    ObjectRendering_stringRender = ObjectRendering_stringRenderArg;
    ObjectRendering_drawVO       = ObjectRendering_drawVOArg;
  
    SurfaceDraw_init(voCtxDynamicArg,phctxArg,trianglesArg,trianglestripArg,linesArg);
    SliderControl_init(voCtxDynamicArg,phctxArg,trianglesArg,trianglestripArg,linestripArg);
    ButtonControl_init(voCtxDynamicArg,phctxArg,trianglesArg,trianglestripArg,linesArg);
    ChannelOccupancyControl_init(triangles,trianglestrip,linestrip,voCtxDynamicArg, fctxArg);
    ScaleControl_init(triangles,trianglestrip,linestrip,linesArg,voCtxDynamicArg, fctxArg,fretctx,reRenderString);
    WidgetsAssemble();
}







int ObjectRendering_getTexture(int idx)
{
    return textures[idx];
}

//Rendering is just drawing all active and renderable items in order
void GenericRendering_draw()
{
    VertexObjectBuilder_reset(voCtxDynamic);    
    
    for(int item=0; item<WidgetTree_count(); item++)
    {
        struct WidgetTree_rect* itemP = WidgetTree_get(item);
        if(itemP->render && itemP->isActive)
        {
            itemP->render(itemP->ctx);            
        }
    }
    ObjectRendering_drawVO(voCtxDynamic);    
}

//Control paging is simply hiding and showing controls
void Page_set(void* ctx, int val)
{
    baseSlider->rect->isActive = FALSE;
    intonationSlider->rect->isActive = FALSE;
    widthSlider->rect->isActive = FALSE;
    heightSlider->rect->isActive = FALSE;
    rootNoteSlider->rect->isActive = FALSE;
    midiChannelSlider->rect->isActive = FALSE;
    midiChannelSpanSlider->rect->isActive = FALSE;
    midiBendSlider->rect->isActive = FALSE;
    octAutoButton->rect->isActive = FALSE;
    legatoButton->rect->isActive = FALSE;
    polyButton->rect->isActive = FALSE;
    baseVolumeSlider->rect->isActive = FALSE;
    scaleControlButton->rect->isActive = FALSE;
    scaleControlButton->val = 0;
    scaleControl->rect->isActive = FALSE;
    scaleClearButton->rect->isActive = FALSE;
    scaleToggleButton->rect->isActive = FALSE;
    scaleCommitButton->rect->isActive = FALSE;
    initialSnapButton->rect->isActive = FALSE;
    snapSpeedSlider->rect->isActive = FALSE;
    engineButton->rect->isActive = FALSE;
    distortionSlider->rect->isActive = FALSE;
    detuneSlider->rect->isActive = FALSE;
    timbreSlider->rect->isActive = FALSE;
    reverbSlider->rect->isActive = FALSE;
    sensitivitySlider->rect->isActive = FALSE;
    switch(val)
    {
        case 0:
            widthSlider->rect->isActive = TRUE;
            heightSlider->rect->isActive = TRUE;
            break;
        case 1:
            intonationSlider->rect->isActive = TRUE;
            rootNoteSlider->rect->isActive = TRUE;
            break;
        case 2:
            midiChannelSlider->rect->isActive = TRUE;
            midiChannelSpanSlider->rect->isActive = TRUE;            
            midiBendSlider->rect->isActive = TRUE;            
            break;
        case 3:
            legatoButton->rect->isActive = TRUE;
            polyButton->rect->isActive = TRUE;
            baseVolumeSlider->rect->isActive = TRUE;
            break;
        case 4:
            scaleControlButton->rect->isActive = TRUE;
            break;
        case 5:
            initialSnapButton->rect->isActive = TRUE;
            snapSpeedSlider->rect->isActive = TRUE;
            engineButton->rect->isActive = TRUE;
            sensitivitySlider->rect->isActive = TRUE;
            break;
        case 6:
            distortionSlider->rect->isActive = TRUE;
            detuneSlider->rect->isActive = TRUE;
            timbreSlider->rect->isActive = TRUE;
            reverbSlider->rect->isActive = TRUE;
            break;
        case 7:
            baseSlider->rect->isActive = TRUE;
            octAutoButton->rect->isActive = TRUE;
            break;
    }
}

void Page_jump_set(void* ctx,int val,int jump)
{
    //The two buttons are always in sync
    currentPage = (currentPage + PAGEMAX + jump) % PAGEMAX;
    Page_set(ctx, currentPage);
}

void Page_next_set(void* ctx,int val)
{
    Page_jump_set(ctx,val,1);
}

void Page_prev_set(void* ctx,int val)
{
    Page_jump_set(ctx,val,-1);
}


int Page_get(void* ctx)
{
    return 0;
}


void NoteDiff_set(void* ctx, float val)
{
    //PitchHandler_setNoteDiff(phctx, 24-1+24*val);
    PitchHandler_setNoteDiff(phctx, val*126);
    SurfaceDraw_drawBackground();
}

float NoteDiff_get(void* ctx)
{
    //return (PitchHandler_getNoteDiff(phctx)-23)/24.0;
    return PitchHandler_getNoteDiff(phctx)/126.0;
}

void Cols_set(void* ctx, float val)
{
    PitchHandler_setColCount(phctx, 5 + val*7);
    SurfaceDraw_drawBackground();
}

float Cols_get(void* ctx)
{
    return (PitchHandler_getColCount(phctx)-5)/7;
}

void Rows_set(void* ctx, float val)
{
    PitchHandler_setRowCount(phctx, 1 + val*6);
    SurfaceDraw_drawBackground();
}

float Rows_get(void* ctx)
{
    return (PitchHandler_getRowCount(phctx)-1)/6;
}

void SnapSpeed_set(void* ctx, float val)
{
    PitchHandler_setTuneSpeed(phctx,val);
}

float SnapSpeed_get(void* ctx)
{
    return PitchHandler_getTuneSpeed(phctx);
}

void Snap_set(void* ctx, int val)
{
    PitchHandler_setSnap(phctx, val);
}

int Snap_get(void* ctx)
{
    return PitchHandler_getSnap(phctx);
}

void Distortion_set(void* ctx, float val)
{
    setDistortion(val);
}

float Distortion_get(void* ctx)
{
    return getDistortion();
}

void Timbre_set(void* ctx, float val)
{
    setTimbre(val);
}

float Timbre_get(void* ctx)
{
    return getTimbre();
}

void Reverb_set(void* ctx, float val)
{
    setReverb(val);
}

float Reverb_get(void* ctx)
{
    return getReverb();
}

void Detune_set(void* ctx, float val)
{
    setDetune(val);
}

float Detune_get(void* ctx)
{
    return getDetune();
}

void Sensitivity_set(void* ctx, float val)
{
    setSensitivity(val);
}

float Sensitivity_get()
{
    return getSensitivity();
}

void MidiBase_set(void* ctx, float val)
{
    int ival = (int)(val*15.99);
    sprintf(stringRenderBuffer,"Channel: %d",ival+1);
    reRenderString(stringRenderBuffer, PIC_MIDIBASETEXT);
    Fretless_setMidiHintChannelBase(fctx, ival);
}

float MidiBase_get(void* ctx)
{
    return Fretless_getMidiHintChannelBase(fctx)/ 15.99;
}

void MidiSpan_set(void* ctx, float val)
{
    int ival = (int)(val*15)+1;
    sprintf(stringRenderBuffer,"Span: %d",ival);
    reRenderString(stringRenderBuffer, PIC_MIDISPANTEXT);
    Fretless_setMidiHintChannelSpan(fctx, ival);
}

float MidiSpan_get(void* ctx)
{
    return (Fretless_getMidiHintChannelSpan(fctx)-1)/ 15.0;
}

void OctAuto_set(void* ctx, int val)
{
    PitchHandler_setOctaveRounding(phctx, val);
}

int OctAuto_get(void* ctx)
{
    return PitchHandler_getOctaveRounding(phctx);
}

void Legato_set(void* ctx, int val)
{
    char cval = "ny"[val];
    sprintf(stringRenderBuffer,"Legato:%c", cval);
    reRenderString(stringRenderBuffer, PIC_LEGATOTEXT);
    return SurfaceTouchHandling_setLegato(val);
}

int Legato_get(void* ctx)
{
    return SurfaceTouchHandling_getLegato();
}

void Poly_set(void* ctx, int val)
{
    if(val == 0)
    {
        reRenderString("Mono", PIC_POLYTEXT);        
    }
    else
    {
        if(val == 1)
        {
            reRenderString("String", PIC_POLYTEXT);                    
        }
        else
        {
            reRenderString("Poly", PIC_POLYTEXT);                                
        }
    }
    return SurfaceTouchHandling_setPoly(val);
}

int Poly_get(void* ctx)
{
    return SurfaceTouchHandling_getPoly();
}

float Vel_get(void* ctx)
{
    return SurfaceTouchHandling_getBaseVolume();
}

void Vel_set(void* ctx, float vel)
{
    SurfaceTouchHandling_setBaseVolume(vel);
}

void MidiBend_set(void* ctx, float val)
{
    int ival = (int)(val*22)+2;
    sprintf(stringRenderBuffer,"Bend: %d", ival);
    reRenderString(stringRenderBuffer, PIC_MIDIBENDTEXT);
    Fretless_setMidiHintChannelBendSemis(fctx, ival);
}

float MidiBend_get(void* ctx)
{
    return (Fretless_getMidiHintChannelBendSemis(fctx)-2)/ 22.0;
}



void Scale_set(void* ctx,int val)
{
    scaleControl->rect->isActive = val;
    scaleClearButton->rect->isActive = val;
    scaleToggleButton->rect->isActive = val;
    scaleCommitButton->rect->isActive = val;
}

int Scale_get(void* ctx)
{
    return 0;
}


void ScaleClear_set(void* ctx,int val)
{
    ScaleControl_clear(ctx);
}

int ScaleClear_get(void* ctx)
{
    return 0;
}

void ScaleToggle_set(void* ctx,int val)
{
    ScaleControl_toggle(ctx);
}

int ScaleToggle_get(void* ctx)
{
    return 0;
}

void ScaleFretDefaults_set(void* ctx,int val)
{
    //struct Fret_context* fretContext = PitchHandler_frets(phctx);
    ScaleControl_defaults(ctx);
}

int ScaleFretDefaults_get(void* ctx)
{
    return 0;
}

void Engine_set(void* ctx,int val)
{
    if(val)
    {
        rawEngineStart();        
    }
    else
    {
        rawEngineStop();
    }
}


void Intonation_set(void* ctx, float val)
{
    int ival = (int)(7.99*val);
    ScaleControl_setCurrentScale(ival);
    intonationSlider->val = val;
}

void RootNote_set(void* ctx, float val)
{
    //Circle of fifths base note, which is an input into Intonation_set
    baseNote = (int)(7*((int)(12*val-4+12)) % 12);
    //This state needs to be maintained because we synthesized it
    rootNoteSlider->val = val;
    
    ScaleControl_setBaseNote(baseNote);
}

//This is called when we have set up the OpenGL context already
void ObjectRendering_loadImages()
{
    
    for(int i=0; i < IMAGECOUNT; i++)
    {
        ObjectRendering_imageRender(
                                    ObjectRendering_imageContext,
                                    requiredTexture[i],
                                    &textures[i],
                                    &textureWidth[i],
                                    &textureHeight[i],
                                    0
                                    );
    }
    
    //Loading up strings because we know that OpenGL context is now valid.
    //This may move to support re-rendering of strings
    renderLabel("MIDI Channels", PIC_CHANNELCYCLINGTEXT);
    renderLabel("Center",PIC_BASENOTETEXT);
    renderLabel("Scale",PIC_SCALETEXT);
    renderLabel("Width",PIC_WIDTHTEXT);
    
    renderLabel("\u219E",PIC_PAGE1TEXT);
    renderLabel("\u21A0",PIC_PAGE2TEXT);
    
    renderLabel("Circle Of Fifths", PIC_ROOTNOTETEXT);
    renderLabel("Height", PIC_HEIGHTTEXT);
    
    renderLabel("Channel", PIC_MIDIBASETEXT);
    MidiBase_set(NULL,MidiBase_get(NULL));
    
    renderLabel("Span", PIC_MIDISPANTEXT);
    MidiSpan_set(NULL,MidiSpan_get(NULL));
    
    renderLabel("Bend", PIC_MIDIBENDTEXT);
    MidiBend_set(NULL,MidiBend_get(NULL));
    
    renderLabel("Distortion", PIC_DISTORTIONTEXT);
    renderLabel("Detune", PIC_DETUNETEXT);
    renderLabel("Timbre", PIC_TIMBRETEXT);
    renderLabel("Space", PIC_REVERBTEXT);
    
    renderLabel("Oct Auto", PIC_OCTTEXT);
    
    renderLabel("Legato:y", PIC_LEGATOTEXT);
    renderLabel("String", PIC_POLYTEXT);
    renderLabel("Velocity", PIC_BASEVOLTEXT);
    
    renderLabel("Re-Fret", PIC_SCALEBUTTONTEXT);
    renderLabel("The Octave", PIC_SCALECONTROLTEXT);
    renderLabel("Clear", PIC_SCALECLEARTEXT);
    renderLabel("Toggle", PIC_SCALETOGGLETEXT);
    renderLabel("Defaults", PIC_SCALEFRETDEFAULTTEXT);
    
    renderLabel("Snap", PIC_INITIALSNAPTEXT);
    renderLabel("Speed", PIC_SNAPSPEEDTEXT);
    renderLabel("Internal", PIC_ENGINETEXT);
    
    renderLabel("Sensitivity", PIC_SENSITIVITY);
    //Render a contiguous group of note pre-rendered images
    //(sharps/flats don't exist for now... a problem I will tackle later)
    for(int n=0; n < 12; n++)
    {
        int i = n + PIC_NOTE0;
        sprintf(stringRenderBuffer,"note-%d",n);
        ObjectRendering_imageRender(
                                    ObjectRendering_imageContext,
                                    stringRenderBuffer,
                                    &textures[i],
                                    &textureWidth[i],
                                    &textureHeight[i],
                                    0
                                    );
    }    
    SurfaceDraw_drawBackground();
}

/**
 *  This assembles all of the controls, and invokes the callbacks, usually locally defined.
 */
void WidgetsAssemble()
{
    struct WidgetTree_rect* itemP = NULL;
    
    itemP = SurfaceDraw_create();    

    //float cx = 0.8;
    //float cy = 0.2;
    
    //Creating a raw control given just a rendering function
    //ChannelOccupancyControl_create(cx - 0.2, cy - 0.2, cx + 0.2, cy + 0.2);
    
    float farLeft = 0.07;
    float farRight = 0.94;
    float panelBottom = 0.9275;
    float panelTop = 1.0;
    //This button cycles through pages of controls
    pagePrevButton = CreateButton(PIC_PAGE1TEXT,0,panelBottom, farLeft,panelTop, Page_prev_set, Page_get,1);
    pageNextButton = CreateButton(PIC_PAGE2TEXT,farRight,panelBottom, 1.0,panelTop, Page_next_set, Page_get,1);
    
    //Page 0
    widthSlider = CreateSlider(PIC_WIDTHTEXT,farLeft,panelBottom, 0.5,panelTop, Cols_set, Cols_get);
    heightSlider = CreateSlider(PIC_HEIGHTTEXT,0.5,panelBottom, farRight,panelTop, Rows_set, Rows_get);
    
    
    //Page 1
    intonationSlider = CreateSlider(PIC_SCALETEXT,farLeft,panelBottom, 0.5,panelTop, Intonation_set, NULL);
    rootNoteSlider = CreateSlider(PIC_ROOTNOTETEXT,0.5,panelBottom, farRight,panelTop, RootNote_set, NULL);
    
    //Page 2
    midiChannelSlider = CreateSlider(PIC_MIDIBASETEXT, farLeft,panelBottom, 0.33,panelTop, MidiBase_set, MidiBase_get);    
    midiChannelSpanSlider = CreateSlider(PIC_MIDISPANTEXT, 0.33,panelBottom, 0.66,panelTop, MidiSpan_set, MidiSpan_get);    
    midiBendSlider = CreateSlider(PIC_MIDIBENDTEXT, 0.66,panelBottom, farRight,panelTop, MidiBend_set, MidiBend_get);

    //Page 3
    legatoButton = CreateButton(PIC_LEGATOTEXT, farLeft, panelBottom, 0.28, panelTop, Legato_set, Legato_get, 2);
    polyButton = CreateButton(PIC_POLYTEXT, 0.28, panelBottom, 0.5, panelTop, Poly_set, Poly_get, 3);
    baseVolumeSlider = CreateSlider(PIC_BASEVOLTEXT, 0.5,panelBottom, farRight,panelTop, Vel_set, Vel_get);
    
    //Page 4
    scaleControlButton = CreateButton(PIC_SCALEBUTTONTEXT, farLeft, panelBottom, 0.28, panelTop, Scale_set, Scale_get, 2);
    scaleControl = ScaleControl_create(0, 0, 1, panelBottom);
    scaleClearButton = CreateButton(PIC_SCALECLEARTEXT,0.28,panelBottom, 0.48,panelTop, ScaleClear_set,ScaleClear_get,1);
    scaleToggleButton = CreateButton(PIC_SCALETOGGLETEXT,0.48,panelBottom, 0.68,panelTop, ScaleToggle_set,ScaleToggle_get,1);    
    scaleCommitButton = CreateButton(PIC_SCALEFRETDEFAULTTEXT,0.68,panelBottom, 0.88,panelTop, ScaleFretDefaults_set,ScaleFretDefaults_get,1);
    
    //Page 5
    initialSnapButton = CreateButton(PIC_INITIALSNAPTEXT, farLeft, panelBottom, 0.25, panelTop, Snap_set, Snap_get, 2);
    snapSpeedSlider = CreateSlider(PIC_SNAPSPEEDTEXT, 0.25, panelBottom, 0.48, panelTop, SnapSpeed_set, SnapSpeed_get);
    sensitivitySlider = CreateSlider(PIC_SENSITIVITY, 0.48, panelBottom, 0.78, panelTop, Sensitivity_set, Sensitivity_get);
    engineButton = CreateButton(PIC_ENGINETEXT, 0.78, panelBottom, farRight, panelTop, Engine_set, NULL,2);
    engineButton->val = 1; //This is in the enabled state from the beginning
    
    //Page 6
    reverbSlider = CreateSlider(PIC_REVERBTEXT,farLeft,panelBottom, 0.30,panelTop, Reverb_set, Reverb_get);
    detuneSlider = CreateSlider(PIC_DETUNETEXT,0.30,panelBottom, 0.48,panelTop, Detune_set, Detune_get);
    timbreSlider = CreateSlider(PIC_TIMBRETEXT,0.48,panelBottom, 0.66,panelTop, Timbre_set, Timbre_get);
    distortionSlider = CreateSlider(PIC_DISTORTIONTEXT,0.66,panelBottom, farRight,panelTop, Distortion_set, Distortion_get);
    
    //Page 7
    octAutoButton = CreateButton(PIC_OCTTEXT,farLeft,panelBottom,farLeft+0.2,panelTop, OctAuto_set, OctAuto_get, 2);
    baseSlider = CreateSlider(PIC_BASENOTETEXT,farLeft+0.2,panelBottom, farRight,panelTop, NoteDiff_set, NoteDiff_get);    
    
    //Page last
    Page_set(NULL, PAGEMAX-1);
    
    ScaleControl_defaults(NULL);
}
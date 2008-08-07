/****************************************************************************
 * Hu-Go!
 *
 * I really wanted to integrate the renderer from GP project, to give me fullscreen
 * RType !
 *
 * This is the 'shagkur patented' scaler in action, yet again :)
 ****************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <sdcard.h>
#include <string.h>
#include "osd_ngc_mix.h"

/*** Using Two framebuffers ***/
unsigned int *xfb[2] = { NULL, NULL };
GXRModeObj *vmode; /*** Generic GXRModeObj ***/
int whichfb = 0;   /*** Remember which buffer is in use ***/

unsigned short RGB565PAL[256];
extern int gamepaused;
extern void DDASampleRate ();

/*** GX Support ***/
#define TEX_WIDTH 512
#define TEX_HEIGHT 512
#define DEFAULT_FIFO_SIZE 256 * 1024

unsigned int copynow = GX_FALSE;
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN (32);
static unsigned char texturemem[TEX_WIDTH * TEX_HEIGHT * 2] ATTRIBUTE_ALIGN (32);
GXTexObj texobj;
static Mtx view;
int vwidth, vheight, oldvwidth, oldvheight;

#define HASPECT 320
#define VASPECT 112

/* 240 lines progressive (NTSC or PAL 60Hz) */
GXRModeObj TVNtsc_Rgb60_240p = 
{
    VI_TVMODE_EURGB60_DS,      // viDisplayMode
    640,             // fbWidth
    240,             // efbHeight
    240,             // xfbHeight
    (VI_MAX_WIDTH_NTSC - 640)/2,        // viXOrigin
    (VI_MAX_HEIGHT_NTSC/2 - 480/2)/2,       // viYOrigin
    640,             // viWidth
    480,             // viHeight
    VI_XFBMODE_SF,   // xFBmode
    GX_FALSE,        // field_rendering
    GX_FALSE,        // aa

     // sample points arranged in increasing Y order
  {
    {6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
    {6,6},{6,6},{6,6},  // pix 1
    {6,6},{6,6},{6,6},  // pix 2
    {6,6},{6,6},{6,6}   // pix 3
  },

     // vertical filter[7], 1/64 units, 6 bits each
  {
      0,         // line n-1
      0,         // line n-1
      21,         // line n
      22,         // line n
      21,         // line n
      0,         // line n+1
      0          // line n+1
  }
};

GXRModeObj *tvmodes[2] =
{
  &TVNtsc_Rgb60_240p, &TVNtsc480IntDf, /* 60hz modes */
};

/* New texture based scaler */
typedef struct tagcamera
{
  Vector pos;
  Vector up;
  Vector view;
} camera;

/*** Square Matrix
     This structure controls the size of the image on the screen.
	 Think of the output as a -80 x 80 by -60 x 60 graph.
***/
s16 square[] ATTRIBUTE_ALIGN (32) =
{
	/*
	 * X,   Y,  Z
	 * Values set are for roughly 4:3 aspect
	 */
	-HASPECT,  VASPECT, 0,	// 0
	 HASPECT,  VASPECT, 0,	// 1
	 HASPECT, -VASPECT, 0,	// 2
	-HASPECT, -VASPECT, 0,	// 3
};

static camera cam = { {0.0F, 0.0F, -100.0F},
{0.0F, -1.0F, 0.0F},
{0.0F, 0.0F, 0.0F}
};


/****************************************************************************
 * VideoThreading
 ****************************************************************************/
#define TSTACK 16384
lwpq_t videoblankqueue;
lwp_t vbthread;
static unsigned char vbstack[TSTACK];

/****************************************************************************
 * vbgetback
 *
 * This callback enables the emulator to keep running while waiting for a
 * vertical blank. 
 *
 * Putting LWP to good use :)
 ****************************************************************************/
static void *
vbgetback (void *arg)
{
  while (1)
  {
    VIDEO_WaitVSync (); /**< Wait for video vertical blank */
    LWP_SuspendThread (vbthread);
  }
}

/****************************************************************************
 * InitVideoThread
 *
 * libOGC provides a nice wrapper for LWP access.
 * This function sets up a new local queue and attaches the thread to it.
 ****************************************************************************/
void
InitVideoThread ()
{
	/*** Initialise a new queue ***/
  LWP_InitQueue (&videoblankqueue);

	/*** Create the thread on this queue ***/
  LWP_CreateThread (&vbthread, vbgetback, NULL, vbstack, TSTACK, 80);
}

/****************************************************************************
 * copy_to_xfb
 *
 * Stock code to copy the GX buffer to the current display mode.
 * Also increments the frameticker, as it's called for each vb.
 ****************************************************************************/
static void
copy_to_xfb ()
{
  if (copynow == GX_TRUE)
    {
      GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
      GX_SetColorUpdate (GX_TRUE);
      GX_CopyDisp (xfb[whichfb], GX_TRUE);
      GX_Flush ();
      copynow = GX_FALSE;
    }

  DDASampleRate ();

}

/****************************************************************************
 * WIP3 - Scaler Support Functions
 ****************************************************************************/
void draw_init (void)
{
 /* Clear all Vertex params */
  GX_ClearVtxDesc ();

  /* Set Position Params (set quad aspect ratio) */
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
  GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
  GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

  /* Set Tex Coord Params */
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
  GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);
  GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
  GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
  GX_SetNumTexGens (1);
  GX_SetNumChans(0);

  /** Set Modelview **/
  memset (&view, 0, sizeof (Mtx));
  guLookAt(view, &cam.pos, &cam.up, &cam.view);
  GX_LoadPosMtxImm (view, GX_PNMTX0);
}

static void draw_vert (u8 pos, f32 s, f32 t)
{
  GX_Position1x8 (pos);
  GX_TexCoord2f32 (s, t);
}

/* textured quad rendering */
static void draw_square ()
{
  GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
  draw_vert (3, 0.0, 0.0);
  draw_vert (2, 1.0, 0.0);
  draw_vert (1, 1.0, 1.0);
  draw_vert (0, 0.0, 1.0);
  GX_End ();
}

/****************************************************************************
 * StartGX
 *
 * This function initialises the GX.
 * WIP3 - Based on texturetest from libOGC examples.
 ****************************************************************************/
void
StartGX (void)
{
  Mtx p;
  GXColor gxbackground = { 0, 0, 0, 0xff };

  /*** Clear out FIFO area ***/
  memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);

  /*** Initialise GX ***/
  GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);
  GX_SetCopyClear (gxbackground, 0x00ffffff);
  GX_SetViewport (0.0F, 0.0F, vmode->fbWidth, vmode->efbHeight, 0.0F, 1.0F);
  GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);
  f32 yScale = GX_GetYScaleFactor(vmode->efbHeight, vmode->xfbHeight);
  u16 xfbHeight = GX_SetDispCopyYScale (yScale);
  GX_SetDispCopySrc (0, 0, vmode->fbWidth, vmode->efbHeight);
  GX_SetDispCopyDst (vmode->fbWidth, xfbHeight);
  GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, GX_TRUE, vmode->vfilter);
  GX_SetFieldMode (vmode->field_rendering, ((vmode->viHeight == 2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
  GX_SetPixelFmt (GX_PF_RGB8_Z24, GX_ZC_LINEAR);
  GX_SetCullMode (GX_CULL_NONE);
  GX_SetDispCopyGamma (GX_GM_1_0);
  GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
  GX_SetColorUpdate (GX_TRUE);
  guOrtho(p, vmode->efbHeight/2, -(vmode->efbHeight/2), -(vmode->fbWidth/2), vmode->fbWidth/2, 100, 1000);
  GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

  /*** Copy EFB -> XFB ***/
  GX_CopyDisp (xfb[whichfb ^ 1], GX_TRUE);
  GX_Flush ();

  /*** Initialize texture data ***/
  memset (texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2);
  
  /*** Force texture update ***/
  vwidth = 100;
  vheight = 100;
}

/****************************************************************************
 * Audio 
 ****************************************************************************/
#define AUDIOBUFFER 2048
unsigned char SoundBuffer[2][AUDIOBUFFER] __attribute__ ((__aligned__ (32)));
int whichab = 0;

/****************************************************************************
 * This is the main callback routine.
 *
 * It is called whenever Audio DMA completes.
 ****************************************************************************/
static void
PCEAudio ()
{

  AUDIO_StopDMA ();
  DCFlushRange (SoundBuffer[whichab], AUDIOBUFFER);
  AUDIO_InitDMA ((u32) SoundBuffer[whichab], AUDIOBUFFER);
  AUDIO_StartDMA ();

  whichab ^= 1;
  memset (SoundBuffer[whichab], 0, AUDIOBUFFER);

  if (!gamepaused)
		/*** Call the Hu-Go! Sound Generator ***/
    MixStereoSound ((short *) SoundBuffer[whichab], AUDIOBUFFER >> 2);
}

void
InitGCAudio ()
{
  AUDIO_Init (NULL);		/*** Start audio subsystem ***/

  memset (SoundBuffer[0], 0, AUDIOBUFFER << 1);

	/*** Set default samplerate to 32khz ***/
  AUDIO_SetDSPSampleRate (AI_SAMPLERATE_32KHZ);

	/*** and the DMA Callback ***/
  AUDIO_RegisterDMACallback (PCEAudio);
  DCFlushRange ((char *) SoundBuffer[0], AUDIOBUFFER);

  PCEAudio ();
}

/****************************************************************************
 * Joypad 
 ****************************************************************************/
#define JOY_A           0x01
#define JOY_B           0x02
#define JOY_SELECT      0x04
#define JOY_RUN 	0x08
#define JOY_UP          0x10
#define JOY_RIGHT       0x20
#define JOY_DOWN        0x40
#define JOY_LEFT        0x80

unsigned short gcpadmap[] = { PAD_BUTTON_B, PAD_BUTTON_A, PAD_TRIGGER_Z,
  PAD_BUTTON_START, PAD_BUTTON_UP, PAD_BUTTON_RIGHT,
  PAD_BUTTON_DOWN, PAD_BUTTON_LEFT
};
int pcemap[] =
  { JOY_B, JOY_A, JOY_SELECT, JOY_RUN, JOY_UP, JOY_RIGHT, JOY_DOWN,
  JOY_LEFT
};
int autofire[2] = { 0, 0 };
int autofireon = 0;

extern void MainMenu ();
int GetJoys (int joy)
{
  int i;
  unsigned short p;
  int ret = 0;
  signed char x, y;
  
  p = PAD_ButtonsHeld (joy);

  for (i = 0; i < 8; i++)
    {
      if (p & gcpadmap[i])
	ret |= pcemap[i];
    }


  if ((p & PAD_TRIGGER_L) || (p & PAD_TRIGGER_R)) MainMenu ();

	/*** And do any Analog fixup ***/
  x = PAD_StickX (joy);
  y = PAD_StickY (joy);

  if (x > 45)
    ret |= JOY_RIGHT;
  if (x < -45)
    ret |= JOY_LEFT;
  if (y > 45)
    ret |= JOY_UP;
  if (y < -45)
    ret |= JOY_DOWN;

	/*** Do autofire check ***/
  p = PAD_ButtonsDown (0);

  autofireon ^= 1;

  if (p & PAD_BUTTON_X)
    autofire[0] ^= 1;

  if (p & PAD_BUTTON_Y)
    autofire[1] ^= 1;

  if (autofireon)
    {
      if (autofire[0])
	ret |= JOY_A;

      if (autofire[1])
	ret |= JOY_B;
    }

  return ret;
}

/****************************************************************************
 * Render Image - New Edition based on GP Renderer
 ****************************************************************************/
void RenderImage (int width, int height, unsigned char *buf, int pitch)
{
  unsigned short *texture;
  int tofs;
  int hpos;
  int y, x;
  
  vwidth = width;
  vheight = height;

  /* Ensure previous vb has complete */
  while ((LWP_ThreadIsSuspended (vbthread) == 0) || (copynow == GX_TRUE))
  {
    usleep (50);
  }

  whichfb ^= 1;

  if ((oldvheight != vheight) || (oldvwidth != vwidth))
  {
    /** Update scaling **/
    oldvwidth = vwidth;
    oldvheight = vheight;

    /* set aspect */
    osd_gfx_init_normal_mode();
	
	/* reinitialize texture */
    GX_InvalidateTexAll ();
    GX_InitTexObj (&texobj, texturemem, vwidth, vheight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
 }

  GX_InvVtxCache ();
  GX_InvalidateTexAll ();

  texture = (unsigned short *) texturemem;
  tofs = 0;

  for (y = 0; y < vheight; y += 4)
  {
    for (x = 0; x < vwidth; x += 4)
    {
      hpos = (y * pitch) + x;
      
      /*** Row One ***/
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
      
      /*** Row Two ***/
      hpos += pitch;
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
      
      /*** Row Three ***/
      hpos += pitch;
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
      
      /*** Row Four ***/
      hpos += pitch;
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
    }
  }
  
  /* load texture into GX */
  DCFlushRange (texturemem, vwidth * vheight * 2);
  GX_LoadTexObj (&texobj, GX_TEXMAP0);
  
  /* render textured quad */
  draw_square ();
  GX_DrawDone ();

  VIDEO_SetNextFramebuffer (xfb[whichfb]);
  VIDEO_Flush ();
  copynow = GX_TRUE;

  /* Return to caller, don't waste time waiting for vb */
  LWP_ResumeThread (vbthread);
}

/****************************************************************************
 * InitGCVideo
 *
 * This function MUST be called at startup.
 ****************************************************************************/
#ifndef HW_RVL
  extern void dvd_drive_detect();
#endif

extern GXRModeObj TVEurgb60Hz480IntDf;
void scanpads (u32 count)
{
	PAD_ScanPads();
}

void InitGCVideo ()
{
  /*
   * Before doing anything else under libogc,
   * Call VIDEO_Init
   */

  VIDEO_Init ();

  /* Get the current video mode then :
      - set menu video mode (fullscreen, 480i or 576i)
      - set emulator rendering TV modes (PAL/MPAL/NTSC/EURGB60)
  */
  switch (VIDEO_GetCurrentTvMode())
  {
#ifndef FORCE_EURGB60
    case VI_MPAL:
      vmode = &TVMpal480IntDf;
      tvmodes[0]->viTVMode = VI_TVMODE_MPAL_DS;
      tvmodes[1]->viTVMode = VI_TVMODE_MPAL_DS;
      break;
    
    case VI_NTSC:
      vmode = &TVNtsc480IntDf;
      tvmodes[0]->viTVMode = VI_TVMODE_NTSC_DS;
      tvmodes[1]->viTVMode = VI_TVMODE_NTSC_INT;
      break;
#endif
   default:
      vmode = &TVEurgb60Hz480IntDf;
      tvmodes[0]->viTVMode = VI_TVMODE_EURGB60_DS;
      tvmodes[1]->viTVMode = VI_TVMODE_EURGB60_INT;
      break;
}

  /* Set default video mode */
  VIDEO_Configure (vmode);

  /* Configure the framebuffers (double-buffering) */
  xfb[0] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(vmode));
  xfb[1] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(vmode));

  /* Define a console */
  console_init(xfb[0], 20, 64, 640, 480, 480 * 2);

  /* Clear framebuffers to black */
  VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);

  /* Set the framebuffer to be displayed at next VBlank */
  VIDEO_SetNextFramebuffer(xfb[0]);

  /* Register Video Retrace handlers */
  VIDEO_SetPreRetraceCallback(copy_to_xfb);
  VIDEO_SetPostRetraceCallback(scanpads);
  
  /* Enable Video Interface */
  VIDEO_SetBlack (FALSE);
  
  /* Update video settings for next VBlank */
  VIDEO_Flush();

  /* Wait for VBlank */
  VIDEO_WaitVSync();
  VIDEO_WaitVSync();

  /* Initialize everything else */
  PAD_Init ();
#ifndef HW_RVL
  DVD_Init ();
  dvd_drive_detect();
#endif

  SDCARD_Init ();
  copynow = GX_FALSE;
  StartGX ();
  
  InitVideoThread ();
  copynow = GX_FALSE;
}

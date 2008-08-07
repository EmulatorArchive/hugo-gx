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
#include <string.h>
#include "osd_ngc_mix.h"

/*** Using Two framebuffers ***/
unsigned int *xfb[2] = { NULL, NULL };	 /*** Framebuffer - used throughout ***/

GXRModeObj *vmode;			       /*** Generic GXRModeObj ***/
int whichfb = 0;			       /*** Remember which buffer is in use ***/

unsigned int gcpalette[256];
unsigned short RGB565PAL[256];
extern int gamepaused;
extern void DDASampleRate ();

/*** GX Support ***/
#define TEX_WIDTH 512
#define TEX_HEIGHT 512
#define WIDTH 640
#define DEFAULT_FIFO_SIZE 256 * 1024
unsigned int copynow = GX_FALSE;
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN (32);
static unsigned char texturemem[TEX_WIDTH * TEX_HEIGHT * 2] ATTRIBUTE_ALIGN (32);
GXTexObj texobj;
static Mtx view;
int vwidth, vheight, oldvwidth, oldvheight;

#define HASPECT 76
#define VASPECT 54

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
	-HASPECT, VASPECT, 0,	// 0
	HASPECT, VASPECT, 0,	// 1
	HASPECT, -VASPECT, 0,	// 2
	-HASPECT, -VASPECT, 0,	// 3
};

static camera cam = { {0.0F, 0.0F, 0.0F},
{0.0F, 0.5F, 0.0F},
{0.0F, 0.0F, -0.5F}
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
      VIDEO_WaitVSync ();	 /**< Wait for video vertical blank */
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
static void
draw_init (void)
{
  GX_ClearVtxDesc ();
  GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
  GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
  GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
  GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

  GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

  GX_SetNumTexGens (1);
  GX_SetTexCoordGen (GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

  GX_InvalidateTexAll ();
  GX_InitTexObj (&texobj, texturemem, vwidth, vheight, GX_TF_RGB565,
		 GX_CLAMP, GX_CLAMP, GX_FALSE);
}

static void
draw_vert (u8 pos, u8 c, f32 s, f32 t)
{
  GX_Position1x8 (pos);
  GX_Color1x8 (c);
  GX_TexCoord2f32 (s, t);
}

static void
draw_square (Mtx v)
{
  Mtx m;			// model matrix.
  Mtx mv;			// modelview matrix.

  guMtxIdentity (m);
  guMtxTransApply (m, m, 0, 0, -100);
  guMtxConcat (v, m, mv);

  GX_LoadPosMtxImm (mv, GX_PNMTX0);
  GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
  draw_vert (0, 0, 0.0, 0.0);
  draw_vert (1, 0, 1.0, 0.0);
  draw_vert (2, 0, 1.0, 1.0);
  draw_vert (3, 0, 0.0, 1.0);
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

  GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
  GX_SetDispCopyYScale ((f32) vmode->xfbHeight / (f32) vmode->efbHeight);
  GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);
  GX_SetDispCopySrc (0, 0, vmode->fbWidth, vmode->efbHeight);
  GX_SetDispCopyDst (vmode->fbWidth, vmode->xfbHeight);
  GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, GX_TRUE,
		    vmode->vfilter);
  GX_SetFieldMode (vmode->field_rendering,
		   ((vmode->viHeight ==
		     2 * vmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
  GX_SetPixelFmt (GX_PF_RGB8_Z24, GX_ZC_LINEAR);
  GX_SetCullMode (GX_CULL_NONE);
  GX_CopyDisp (xfb[whichfb ^ 1], GX_TRUE);
  GX_SetDispCopyGamma (GX_GM_1_0);

  guPerspective (p, 60, 1.33F, 10.0F, 1000.0F);
  GX_LoadProjectionMtx (p, GX_PERSPECTIVE);
  memset (texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2);
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
int
GetJoys (int joy)
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

  if ((p & PAD_TRIGGER_L) && (p & PAD_TRIGGER_R))
    MainMenu ();

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
void
RenderImage (int width, int height, unsigned char *buf, int pitch)
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
      draw_init ();
      memset (&view, 0, sizeof (Mtx));
	  guLookAt(view, &cam.pos, &cam.up, &cam.view);
      GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
  }

  GX_InvVtxCache ();
  GX_InvalidateTexAll ();
  GX_SetTevOp (GX_TEVSTAGE0, GX_DECAL);
  GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

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

  DCFlushRange (texturemem, TEX_WIDTH * TEX_HEIGHT * 2);

  GX_SetNumChans (1);

  GX_LoadTexObj (&texobj, GX_TEXMAP0);

  draw_square (view);

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
void
InitGCVideo ()
{
  /*
   * Before doing anything else under libogc,
   * Call VIDEO_Init
   */

  VIDEO_Init ();
  PAD_Init ();

  /*
   * Detect and reset the video mode
   * This is always set to 60hz
   * Whether your running PAL or NTSC
   */

  switch (VIDEO_GetCurrentTvMode ())
    {
    case VI_NTSC:
      vmode = &TVNtsc480IntDf;
      break;

    case VI_PAL:
    case VI_MPAL:
      vmode = &TVMpal480IntDf;
      break;

    default:
      vmode = &TVNtsc480IntDf;
      break;
    }

#ifdef FORCE_PAL50
  vmode = &TVPal528IntDf;
#endif
  VIDEO_Configure (vmode);

   /*** Now configure the framebuffer. 
	     Really a framebuffer is just a chunk of memory
	     to hold the display line by line.
   **/

  xfb[0] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(vmode));

  /*** I prefer also to have a second buffer for double-buffering.
	     This is not needed for the console demo.
   ***/
  xfb[1] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(vmode));

  /*** Define a console ***/
  console_init(xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight, vmode->fbWidth * 2);

  /*** Clear framebuffer to black ***/
  VIDEO_ClearFrameBuffer(vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer(vmode, xfb[1], COLOR_BLACK);

  /*** Set the framebuffer to be displayed at next VBlank ***/
  VIDEO_SetNextFramebuffer(xfb[0]);

  /*** Increment frameticker and timer ***/
  VIDEO_SetPreRetraceCallback(copy_to_xfb);

  /*** Get the PAD status updated by libogc ***/
  VIDEO_SetPostRetraceCallback(PAD_ScanPads);
  VIDEO_SetBlack (FALSE);
  
  /*** Update the video for next vblank ***/
  VIDEO_Flush();

  VIDEO_WaitVSync();		/*** Wait for VBL ***/
  if (vmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

  DVD_Init ();
  copynow = GX_FALSE;
  StartGX ();

  InitVideoThread ();
  copynow = GX_FALSE;
  
  /*
   * Finally, the video is up and ready for use :)
   */
}

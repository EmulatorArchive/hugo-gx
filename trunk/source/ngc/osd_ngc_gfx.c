/****************************************************************************
 * Hu-Go! Nintendo Gamecube
 *
 * GFX functions
 ****************************************************************************/

#include "defs.h"
#include "sys_gfx.h"
#include "hard_pce.h"
#include "gfx.h"

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

//! PC Engine rendered screen
unsigned char *screen = NULL;

//! Host machine rendered screen
unsigned char *physical_screen = NULL;

// COLOR palette
unsigned short RGB565PAL[256];

extern int gcpalette[256];
extern unsigned short RGB565PAL[256];

int osd_gfx_init();
int osd_gfx_init_normal_mode();
void osd_gfx_put_image_normal();
void osd_gfx_shut_normal_mode();

/*** VI ***/
unsigned int *xfb[2];	/*** Double buffered            ***/
int whichfb = 0;		  /*** External framebuffer index ***/
GXRModeObj *vmode;    /*** Menu video mode            ***/

/*** GX ***/
#define TEX_WIDTH         512
#define TEX_HEIGHT        512
#define DEFAULT_FIFO_SIZE 256 * 1024
#define HASPECT           320
#define VASPECT           112

static u8 gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN (32);
static u8 texturemem[TEX_WIDTH * (TEX_HEIGHT + 8) * 2] ATTRIBUTE_ALIGN (32);
static GXTexObj texobj;
static Mtx view;
static int texwidth, texheight, oldvwidth, oldvheight;

int frameticker;

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

/* TV Modes table */
GXRModeObj *tvmodes[2] =
{
  &TVNtsc_Rgb60_240p, &TVNtsc480IntDf, /* 60hz modes */
};

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
static s16 square[] ATTRIBUTE_ALIGN (32) =
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

static camera cam = {
  {0.0F, 0.0F, -100.0F},
  {0.0F, -1.0F, 0.0F},
  {0.0F, 0.0F, 0.0F}
};

/* rendering initialization */
/* should be called each time you change quad aspect ratio */
static void draw_init(void)
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

/* vertex rendering */
static void draw_vert(u8 pos, f32 s, f32 t)
{
  GX_Position1x8 (pos);
  GX_TexCoord2f32 (s, t);
}

/* textured quad rendering */
static void draw_square (void)
{
  GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
  draw_vert (3, 0.0, 0.0);
  draw_vert (2, 1.0, 0.0);
  draw_vert (1, 1.0, 1.0);
  draw_vert (0, 0.0, 1.0);
  GX_End ();
}

/* retrace handler */
extern void DDASampleRate ();

static void framestart(u32 retraceCnt)
{
  /* simply increment the tick counter */
  frameticker++;
  DDASampleRate ();
}

static void gxStart(void)
{
  Mtx p;
  GXColor gxbackground = { 0, 0, 0, 0xff };

  /*** Clear out FIFO area ***/
  memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);

  /*** GX default ***/
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

  /*** reset XFB ***/
  GX_CopyDisp (xfb[whichfb ^ 1], GX_TRUE);

  /*** Initialize texture data ***/
  memset (texturemem, 0, TEX_WIDTH * TEX_HEIGHT * 2);

  /*** Force texture update ***/
  texwidth = 100;
  texheight = 100;
}


u8 render = 0;
u8 aspect = 1;

/* Reinitialize GX */
void ogc_video__reset()
{
	GXRModeObj *rmode;
	Mtx p;

  /* reset scaler */
  osd_gfx_init_normal_mode();

  /* reinitialize current TV mode */
  if (render == 2)
  {
    tvmodes[1]->viTVMode = VI_TVMODE_NTSC_PROG;
    tvmodes[1]->xfbMode = VI_XFBMODE_SF;
  }
  else
  {
    tvmodes[1]->viTVMode = tvmodes[0]->viTVMode & ~3;
    tvmodes[1]->xfbMode = VI_XFBMODE_DF;
  }

	rmode = render ? tvmodes[1] : tvmodes[0];

	VIDEO_Configure (rmode);
	VIDEO_ClearFrameBuffer(rmode, xfb[whichfb], COLOR_BLACK);
	VIDEO_Flush();
	VIDEO_WaitVSync();

  /* reset rendering mode */
  GX_SetViewport (0.0F, 0.0F, rmode->fbWidth, rmode->efbHeight, 0.0F, 1.0F);
  GX_SetScissor (0, 0, rmode->fbWidth, rmode->efbHeight);
  f32 yScale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
  u16 xfbHeight = GX_SetDispCopyYScale (yScale);
  GX_SetDispCopySrc (0, 0, rmode->fbWidth, rmode->efbHeight);
  GX_SetDispCopyDst (rmode->fbWidth, xfbHeight);
  GX_SetCopyFilter (rmode->aa, rmode->sample_pattern, render ? GX_TRUE : GX_FALSE, rmode->vfilter);
  GX_SetFieldMode (rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
  GX_SetPixelFmt (GX_PF_RGB8_Z24, GX_ZC_LINEAR);
  guOrtho(p, rmode->efbHeight/2, -(rmode->efbHeight/2), -(rmode->fbWidth/2), rmode->fbWidth/2, 100, 1000);
  GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);
}

extern GXRModeObj TVEurgb60Hz480IntDf;

/* Initialize VIDEO subsystem */
void ogc_video__init(void)
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
  vmode = VIDEO_GetPreferredMode(NULL);
  if ((vmode->viTVMode >> 2) == VI_PAL) vmode = &TVEurgb60Hz480IntDf;

  /* set GX rendering modes */
  tvmodes[0]->viTVMode = VI_TVMODE(vmode->viTVMode >> 2, VI_NON_INTERLACE);
  tvmodes[1]->viTVMode = VI_TVMODE(vmode->viTVMode >> 2, VI_INTERLACE);

#ifndef HW_RVL
  /* force 480p on NTSC GameCube if the Component Cable is present */
  if (VIDEO_HaveComponentCable()) vmode = &TVNtsc480Prog;
#endif

/* set default rendering mode */
  render = (vmode->viTVMode == VI_TVMODE_NTSC_PROG) ? 2 : 0;
   
  /* configure video mode */
  VIDEO_Configure (vmode);

  /* Configure the framebuffers (double-buffering) */
  xfb[0] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(vmode));
  xfb[1] = (u32 *) MEM_K0_TO_K1((u32 *) SYS_AllocateFramebuffer(vmode));

  /* Define a console */
  console_init(xfb[0], 20, 64, 640, 480, 480 * 2);

  /* Clear framebuffers to black */
  VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);

  /* Set the framebuffer to be displayed at next VBlank */
  VIDEO_SetNextFramebuffer (xfb[0]);

  /* Register Video Retrace handlers */
  VIDEO_SetPreRetraceCallback(framestart);

  /* Enable Video Interface */
  VIDEO_SetBlack (FALSE);
  
  /* Update video settings for next VBlank */
  VIDEO_Flush ();

  /* Wait for VBlank */
  VIDEO_WaitVSync();
  VIDEO_WaitVSync();

  /* Initialize GX */
  gxStart();
}


/* Hu-Go! default */
osd_gfx_driver osd_gfx_driver_list[3] =
{
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode },
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode },  
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode }
};

/*****************************************************************************

    Function: osd_gfx_put_image_normal

    Description: draw the raw computed picture to screen, without any effect
       trying to center it (I bet there is still some work on this, maybe not
                            in this function)
    Parameters: none
    Return: nothing

*****************************************************************************/
void osd_gfx_put_image_normal(void)
{
  unsigned short *texture;
  int tofs;
  int hpos;
  int y, x;

  u8 *buf = (u8 *)osd_gfx_buffer;

  texheight = io.screen_h;
  texwidth  = io.screen_w;
  
  /* check if viewport has changed */
  if ((oldvheight != texheight) || (oldvwidth != texwidth))
  {
    /** Update scaling **/
    oldvwidth = texwidth;
    oldvheight = texheight;

    /* set aspect */
    osd_gfx_init_normal_mode();
	
	  /* reinitialize texture */
    GX_InvalidateTexAll ();
    GX_InitTexObj (&texobj, texturemem, texwidth, texheight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
  }

  GX_InvVtxCache ();
  GX_InvalidateTexAll ();
 
  texture = (unsigned short *) texturemem;
  tofs = 0;

  for (y = 0; y < texheight; y += 4)
  {
    for (x = 0; x < texwidth; x += 4)
    {
      hpos = (y * XBUF_WIDTH) + x;
      
      /*** Row One ***/
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
      
      /*** Row Two ***/
      hpos += XBUF_WIDTH;
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
      
      /*** Row Three ***/
      hpos += XBUF_WIDTH;
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
      
      /*** Row Four ***/
      hpos += XBUF_WIDTH;
      texture[tofs++] = RGB565PAL[buf[hpos]];
      texture[tofs++] = RGB565PAL[buf[hpos + 1]];
      texture[tofs++] = RGB565PAL[buf[hpos + 2]];
      texture[tofs++] = RGB565PAL[buf[hpos + 3]];
    }
  }

  /* load texture into GX */
  DCFlushRange (texturemem, texwidth * texheight * 2);
  GX_LoadTexObj (&texobj, GX_TEXMAP0);
  
  /* render textured quad */
  draw_square ();
  GX_DrawDone ();

  /* switch external framebuffers then copy EFB to XFB */
  whichfb ^= 1;
  GX_CopyDisp (xfb[whichfb], GX_TRUE);
  GX_Flush ();

  /* set next XFB */
  VIDEO_SetNextFramebuffer (xfb[whichfb]);
  VIDEO_Flush ();
}

/*****************************************************************************

    Function: osd_gfx_set_message

    Description: compute the message that will be displayed to create a sprite
       to blit on screen
    Parameters: char* mess, the message to display
    Return: nothing but set OSD_MESSAGE_SPR

*****************************************************************************/
void osd_gfx_set_message(char* mess)
{
	/*** TODO: Update the screen info ***/
}

/*
 * osd_gfx_init:
 * One time initialization of the main output screen
 */
int osd_gfx_init(void)
{
	SetPalette();
  return 1;
}


/*****************************************************************************

    Function:  osd_gfx_init_normal_mode

    Description: initialize the classic 256*224 video mode for normal video_driver
    Parameters: none
    Return: 0 on error
            1 on success

*****************************************************************************/
int osd_gfx_init_normal_mode()
{
  int yscale;
  if (!p_io) return 0;

  /* update  aspect */
  if (aspect)  yscale = render ? io.screen_h : (io.screen_h / 2);
  else yscale = render ? 224 : 112;
  
  square[4] = square[1]  =  yscale;
  square[7] = square[10] = -yscale;
  draw_init();
  return 1;  
}

//! Delete the window
void osd_gfx_shut_normal_mode(void)
{
}

/*****************************************************************************

    Function: osd_gfx_set_color

    Description: Change the component of the choosen color
    Parameters: UChar index : index of the color to change
    			UChar r	: new red component of the color
                UChar g : new green component of the color
                UChar b : new blue component of the color
    Return:

*****************************************************************************/
void osd_gfx_set_color(UChar index, UChar r, UChar g, UChar b)
{
  r <<= 2;
  g <<= 2;
  b <<= 2;

  RGB565PAL[index] = ( ( r << 8 ) & 0xf800 ) | ( ( g << 3 ) & 0x7e0 ) | ( b >> 3 ) ;
}

/****************************************************************************
 * Hu-Go! Nintendo Gamecube
 *
 * GFX functions
 ****************************************************************************/
#include "pce.h"
#include "sys_dep.h"

//! PC Engine rendered screen
unsigned char *screen = NULL;

//! Host machine rendered screen
unsigned char *physical_screen = NULL;

extern int gcpalette[256];
extern unsigned short RGB565PAL[256];
extern void RenderImage( int width, int height, char *src, int pitch );

int osd_gfx_init();
int osd_gfx_init_normal_mode();
void osd_gfx_put_image_normal();
void osd_gfx_shut_normal_mode();

void osd_gfx_dummy_func();

osd_gfx_driver osd_gfx_driver_list[3] =
{
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode },
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode },  
  { osd_gfx_init, osd_gfx_init_normal_mode, osd_gfx_put_image_normal, osd_gfx_shut_normal_mode }
};

void osd_gfx_dummy_func(void)
{
 return;
}

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
	/* The screen is held in osd_gfx_buffer + y * XBUF_WIDTH
	   Actual dimensions are held in io.screen_w, io.screen_h */
	RenderImage( io.screen_w, io.screen_h, osd_gfx_buffer, XBUF_WIDTH);
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
extern void SetPalette (void);
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
unsigned char use_480i = 0;  /* progressive modes */
unsigned char aspect   = 1;	 /* original aspect */
extern signed short square[];
extern void draw_init (void);

int osd_gfx_init_normal_mode()
{
  int yscale;

  if (!p_io) return 0;

  /* update  aspect */
  if (aspect)  yscale = use_480i ? io.screen_h : (io.screen_h / 2);
  else yscale = use_480i ? 224 : 112;
  
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
void osd_gfx_set_color(UChar index,
                       UChar r,
                       UChar g,
                       UChar b)
{
  r <<= 2;
  g <<= 2;
  b <<= 2;

  RGB565PAL[index] = ( ( r << 8 ) & 0xf800 ) | ( ( g << 3 ) & 0x7e0 ) | ( b >> 3 ) ;
}


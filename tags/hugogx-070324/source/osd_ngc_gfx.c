/****************************************************************************
 * Hu-Go! Nintendo Gamecube
 *
 * GFX functions
 ****************************************************************************/
#include "cleantyp.h"
#include "pce.h"
#include "sys_dep.h"

//! PC Engine rendered screen
unsigned char *screen = NULL;

//! Host machine rendered screen
unsigned char *physical_screen = NULL;

int blit_x,blit_y;
// where must we blit the screen buffer on screen

int screen_blit_x, screen_blit_y;
// where on the screen we must blit XBuf

UChar* XBuf;
// buffer for video flipping

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
  return 1;  
}


//! Delete the window
void osd_gfx_shut_normal_mode(void)
{
}

/****************************************************************************
 * rgbcolor
 *
 * Support routine for gcpalette
 ****************************************************************************/

unsigned int rgbcolor( unsigned char r1, unsigned char g1, unsigned char b1,
		   unsigned char r2, unsigned char g2, unsigned char b2)
{
  	int y1,cb1,cr1,y2,cb2,cr2,cb,cr;

	y1=(299*r1+587*g1+114*b1)/1000;
	cb1=(-16874*r1-33126*g1+50000*b1+12800000)/100000;
	cr1=(50000*r1-41869*g1-8131*b1+12800000)/100000;

	y2=(299*r2+587*g2+114*b2)/1000;
	cb2=(-16874*r2-33126*g2+50000*b2+12800000)/100000;
	cr2=(50000*r2-41869*g2-8131*b2+12800000)/100000;

	cb=(cb1+cb2) >> 1;
	cr=(cr1+cr2) >> 1;

	return ( (y1 << 24) | (cb << 16) | (y2 << 8) | cr );
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

  gcpalette[index] = rgbcolor(r, g, b, r, g, b);
  RGB565PAL[index] = ( ( r << 8 ) & 0xf800 ) 
		   | ( ( g << 3 ) & 0x7e0 )
                   | ( b >> 3 ) ;

}


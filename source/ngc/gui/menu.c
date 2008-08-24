/****************************************************************************
 * Nintendo Gamecube User Menu
 ****************************************************************************/
#include <pce.h>
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "hugologo.h"
#include "font.h"
#include "dvd.h"

#ifdef HW_RVL
#include <wiiuse/wpad.h>
#include "di/di.h"
#endif

extern unsigned int *xfb[2];
extern int whichfb;
extern GXRModeObj *vmode;
extern int copynow;
extern short ogc_input__getMenuButtons();
extern void ogc_video__reset();
extern void ResetSound();
extern int hugoromsize;
unsigned int savetimer;

unsigned int *backdrop;
unsigned int backcolour;
char version[] = { "Version 2.12" };

/****************************************************************************
 * Unpack Background
 ****************************************************************************/
void unpack()
{
	unsigned long res, inbytes, outbytes;
	int *temp;
	int h,w,v;

	inbytes = hugologo_COMPRESSED;
	outbytes = hugologo_RAW;

	/*** Allocate the background bitmap ***/
	backdrop = malloc( 320 * 480 * 4 );

	/*** Allocate temporary space ***/
	temp = malloc(outbytes + 16);
	res = uncompress( (Bytef *)temp, &outbytes, (Bytef *)hugologo, inbytes);

	memcpy(&w, temp, 4);
	backcolour = w;

	/*** Fill backdrop ***/
	for (h = 0; h < (320 * 480); h++) backdrop[h] = w;

	/*** Put picture in position ***/
	v = 0;
	for (h = 0; h < hugologo_HEIGHT; h++)
	{
		for (w = 0; w < hugologo_WIDTH >> 1; w++)
			backdrop[((h + 40) * 320) + w + 89] = temp[v++];		
	}

	free( temp );
	init_font();
}

void copybackdrop()
{
	whichfb ^= 1;
	memcpy(xfb[whichfb], backdrop, 320 * 480 * 4);
}

/****************************************************************************
 *  Generic Display functions
 *
 ****************************************************************************/
void ShowScreen()
{
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
}

void ClearScreen()
{
	whichfb ^= 1;
	VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], backcolour);
}

void WaitPrompt( char *prompt )
{
	copybackdrop();
	write_centre( 280, prompt );
	write_centre( 280 + font_height, "Press A to continue");
	ShowScreen();
	while (!(ogc_input__getMenuButtons() & PAD_BUTTON_A));
	while (ogc_input__getMenuButtons() & PAD_BUTTON_A);
}

void ShowAction(char *prompt)
{
	copybackdrop();
	write_centre(290, prompt);
	ShowScreen();
}

/****************************************************************************
 * Logo Pour
 *
 * Just a little effect to keep me interested :)
 ****************************************************************************/
void pourlogo()
{
	int i;
	int w;
	int h;
	int v;
	int linecount = 0;

	/*** Pour in the logo ***/
	memcpy(&w, backdrop, 4);

	for (i = 0; i < 100; i++)
	{
		whichfb ^= 1;
		for (h = 0; h < ( 320 * 480 ); h++) xfb[whichfb][h] = w;

		/*** Now pour 4 lines at a time ***/
		/*** Copy base on screen ***/
		if (linecount)
		{
			memcpy(&xfb[whichfb][((334 - linecount) * 320)], 
			       backdrop +((228 - linecount) * 320),
			       linecount * 4 * 320);

			for ( v = 0; v < (334-linecount); v++)
				memcpy(&xfb[whichfb][v * 320], backdrop + ((228-linecount) * 320), 320 * 4);
		}

		linecount += 2;
		write_centre(340, version);
		ShowScreen();
	}

	sleep(1);
}

/****************************************************************************
 * Credits
 ****************************************************************************/
void credits()
{
	int p = 220 + ( font_height << 1);
	copybackdrop();
	write_centre(p, "Hu-Go! - Zeograd http://www.zeograd.com");
	p += font_height;
	write_centre(p, "PCE PSG Info - Paul Clifford / John Kortink / Ki");
	p += font_height;
	write_centre(p, "Gamecube & Wii Port - softdev / eke-eke");
	p += font_height;
	write_centre(p, "libOGC - shagkur / wntrmute");
	p += ( font_height << 1 );
	write_centre(p, "Support - http://www.tehskeen.com");
	ShowScreen();
	sleep(1);
}

/****************************************************************************
 * Menu
 ****************************************************************************/
static int menu = 0;

void DrawMenu( char items[][20], int maxitems, int selected )
{
	int i,p;

  copybackdrop();
  write_centre( 210, version);
	p = 210 + (font_height << 1);
	for ( i = 0 ; i < maxitems ; i++ )
	{
		if ( i == selected ) write_centre_hi( p, items[i] );
		else write_centre( p, items[i] );
		p += font_height;
	}

	ShowScreen();
}

int DoMenu (char items[][20], int maxitems)
{
  int redraw = 1;
  int quit = 0;
  short p;
  int ret = 0;

  while (quit == 0)
  {
    if (redraw)
	  {
      DrawMenu (&items[0], maxitems, menu);
      redraw = 0;
	  }

    p = ogc_input__getMenuButtons();

	  /*** Look for up ***/
    if (p & PAD_BUTTON_UP)
	  {
	    redraw = 1;
	    menu--;
      if (menu < 0) menu = maxitems - 1;
	  }

	  /*** Look for down ***/
    if (p & PAD_BUTTON_DOWN)
	  {
	    redraw = 1;
	    menu++;
      if (menu == maxitems) menu = 0;
	  }

    if (p & PAD_BUTTON_A)
	  {
      quit = 1;
      ret = menu;
	  }
	
	  if (p & PAD_BUTTON_LEFT)
	  {
	    quit = 1;
	    ret = 0 - 2 - menu;
	  }
	
    if (p & PAD_BUTTON_B)
	  {
	    quit = 1;
	    ret = -1;
	  }
  }

  return ret;
}


/****************************************************************************
 * WRAM Menu
 ****************************************************************************/
static u8 device = 0;
extern int ManageWRAM(u8 direction, u8 device);

int wrammenu ()
{
	int prevmenu = menu;
	int quit = 0;
	int ret;
	int count = 4;
	char items[4][20];

	sprintf(items[1], "Load WRAM");
	sprintf(items[2], "Save WRAM");
	sprintf(items[3], "Return to previous");

	menu = 2;

	while (quit == 0)
	{
    if (device == 0) sprintf(items[0], "Device: SDCARD");
    else if (device == 1) sprintf(items[0], "Device: MCARD A");
    else if (device == 2) sprintf(items[0], "Device: MCARD B");

    ret = DoMenu (&items[0], count);
		switch (ret)
		{
			case -1:
      case 3:
				quit = 1;
				break;

			case 0:
        device = (device + 1)%3;
				break;

      case 1:
      case 2:
        quit = ManageWRAM(ret-1,device);
        if (quit) return 1;
        break;
		}

	}

	menu = prevmenu;
	return 0;
}
/****************************************************************************
 * OPTION Menu
 ****************************************************************************/
extern u8 aspect;
extern u8 render;

int optionmenu ()
{
	int prevmenu = menu;
	int quit = 0;
	int ret;
	int count = 4;
	char items[4][20];

	menu = 2;

	while (quit == 0)
	{
		sprintf(items[0], "Aspect: %s", aspect ? "ORIGINAL" : "STRETCH");
		if (render == 1) sprintf (items[1], "Render: BILINEAR");
		else if (render == 2) sprintf (items[1], "Render: PROGRESS");
		else sprintf (items[1], "Render: ORIGINAL");
    sprintf(items[2], "Country: %s", Country ? "JAP" : "USA");
    sprintf(items[3], "Force US Card: %s", US_encoded_card ? "Y" : "N");

		ret = DoMenu (&items[0], count);
		switch (ret)
		{
			case -1:
				quit = 1;
				break;

			case 0:
				aspect ^= 1;
				break;

			case 1:
				render = (render + 1) % 3;
				if (render == 2)
				{
					if (!VIDEO_HaveComponentCable())
          {
            /* do nothing if component cable is not detected */
            render = 0;
          }
				}

			case 2: // Console Type: TG-16 (USA) or PC-Engine
				Country ^= 1;
        break;

      case 3: // Card Encoding
        US_encoded_card ^= 1; 
				if (!cart_reload && hugoromsize)
				{
					ResetPCE();
					ResetSound();
					savetimer = 0;
					quit = 1;
				}
				break;
		}
	}

	menu = prevmenu;
	return 0;
}
			

/****************************************************************************
 * Load Rom menu
 *
 ****************************************************************************/
extern int OpenSD ();
extern int OpenDVD ();
extern int OpenHistory();
static u8 load_menu = 0;

void loadmenu ()
{
	int ret;
  int quit = 0;
  int count = 5;
  char item[5][20] = {
		{"Load Recent"},
		{"Load from SDCARD"},
    {"Load from DVD"},
    {"Stop DVD Motor"},
    {"Return to previous"}
  };

  menu = load_menu;

  while (quit == 0)
  {
    ret = DoMenu(&item[0], count);

    switch (ret)
    {
      case -1:
      case  4:
        quit = 1;
        break;
      
			case 0: /*** Load Recent ***/
				quit = OpenHistory();
        break;
      
      case 1: /*** Load from SCDARD ***/
				quit = OpenSD();
				break;

      case 2:	 /*** Load from DVD ***/
  			quit = OpenDVD();
        break;
  
      case 3:  /*** Stop DVD Disc ***/
        dvd_motor_off();
				break;
    }
  }
  
  load_menu = menu;
}

/****************************************************************************
 * Main Menu
 *
 ****************************************************************************/
extern int frameticker;
int gamepaused = 0;

void MainMenu()
{
	s8 ret;
	u8 quit = 0;
  menu = 0;
	u8 count = 7;
	char items[7][20] =
	{
		{"Play Game"},
		{"Hard Reset"},
		{"Load New Game"},
		{"Emulator Options"},
		{"WRAM Manager"},
		{"Return to Loader"},
		{"System Reboot"}
	};

	savetimer = timer_60;
	gamepaused = 1;

	/* Switch to menu default rendering mode (60hz or 50hz, but always 480 lines) */
	VIDEO_Configure (vmode);
	VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], COLOR_BLACK);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();

	while (quit == 0)
	{
		ret = DoMenu (&items[0], count);

		switch (ret)
		{
			case -1:
			case  0:
				if (hugoromsize) quit = 1;
				break;

			case 1:
				if (!cart_reload && hugoromsize)
				{
					ResetPCE();
					ResetSound();
					savetimer = 0;
					quit = 1;
				}
				break;

			case 2:
        loadmenu();
        menu = 0;
				break;	

			case 3:
				optionmenu();
				break;

   			case 4 :
				quit = wrammenu();
				break;

			case 5: 
        VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], COLOR_BLACK);
        VIDEO_Flush();
        VIDEO_WaitVSync();
#ifdef HW_RVL
        DI_Close();
#endif
        exit(0);
				break;

			case 6:
        VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], COLOR_BLACK);
        VIDEO_Flush();
        VIDEO_WaitVSync();
#ifdef HW_RVL
        DI_Close();
				SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
#else
				SYS_ResetSystem(SYS_HOTRESET,0,0);
#endif
				break;
		}
	}

	/*** Remove any still held buttons ***/
	while(PAD_ButtonsHeld(0)) PAD_ScanPads();
#ifdef HW_RVL
  while(WPAD_ButtonsHeld(0)) WPAD_ScanPads();
#endif

	/*** Reinitialize current TV mode ***/
  ogc_video__reset();
  
  gamepaused = 0;
	timer_60 = savetimer;
  frameticker = 0;

#ifndef HW_RVL
  /*** Stop the DVD from causing clicks while playing ***/
  uselessinquiry ();
#endif
}

/****************************************************************************
 * Nintendo Gamecube User Menu
 ****************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "hugologo.h"
#include "font.h"
#include "dvd.h"

#define PSOSDLOADID 0x7c6000a6

extern unsigned int *xfb[2];
extern int whichfb;
extern GXRModeObj *vmode;
extern int copynow;

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
	res = uncompress( (char *)temp, &outbytes, (char *)hugologo, inbytes);

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

void ShowScreen()
{
        VIDEO_SetNextFramebuffer(xfb[whichfb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
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

	for (i = 0; i < 200; i++) VIDEO_WaitVSync();
	
}

/****************************************************************************
 * Credits
 ****************************************************************************/
void credits()
{
	int p = 220 + ( font_height << 1);
	while ( PAD_ButtonsDown(0) & PAD_BUTTON_A );
	copybackdrop();
	write_centre(p, "Hu-Go! - Zeograd http://www.zeograd.com");
	p += font_height;
	write_centre(p, "PCE PSG Info - Paul Clifford / John Kortink / Ki");
	p += font_height;
	write_centre(p, "Gamecube Port - softdev");
	p += font_height;
	write_centre(p, "libOGC - shagkur / wntrmute");
	p += ( font_height << 1 );
	write_centre(p, "Support - http://www.tehskeen.com");
	ShowScreen();
	while ( !(PAD_ButtonsDown(0) & PAD_BUTTON_A ) );
}

/****************************************************************************
 * Menu
 ****************************************************************************/
static int menu = 0;

void DrawMenu( char items[][20], int maxitems, int selected )
{
	int i,p;

    copybackdrop();
    write_centre( 230, version);
	p = 230 + (font_height << 1);
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
  signed char a,b;

  while (quit == 0)
  {
      if (redraw)
	  {
	      DrawMenu (&items[0], maxitems, menu);
	      redraw = 0;
	  }

      p = PAD_ButtonsDown (0);
      a = PAD_StickY (0);
	  b = PAD_StickX (0);

	  /*** Look for up ***/
      if ((p & PAD_BUTTON_UP) || (a > 70))
	  {
	    redraw = 1;
	    menu--;
        if (menu < 0) menu = maxitems - 1;
	  }

	  /*** Look for down ***/
      if ((p & PAD_BUTTON_DOWN) || (a < -70))
	  {
	    redraw = 1;
	    menu++;
        if (menu == maxitems) menu = 0;
	  }

      if ((p & PAD_BUTTON_A) || (b > 40) || (p & PAD_BUTTON_RIGHT))
	  {
		quit = 1;
		ret = menu;
	  }
	
	  if ((b < -40) || (p & PAD_BUTTON_LEFT))
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
 * WaitPrompt
 ****************************************************************************/
void WaitPrompt( char *prompt )
{
	copybackdrop();
	write_centre( 280, prompt );
	write_centre( 280 + font_height, "Press A to continue");
	ShowScreen();
	while (!(PAD_ButtonsDown(0) & PAD_BUTTON_A));
	while (PAD_ButtonsDown(0) & PAD_BUTTON_A);
}

void ShowAction( char *prompt )
{
	copybackdrop();
	write_centre(290, prompt);
	ShowScreen();
}

void ClearScreen()
{
	whichfb ^= 1;
	VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], backcolour);
}

/****************************************************************************
 * WRAM Menu
 ****************************************************************************/
int CARDSLOT = CARD_SLOTB;
int use_SDCARD = 0;
extern int doStateSaveLoad(int which);

int wrammenu ()
{
	int prevmenu = menu;
	int quit = 0;
	int ret;
	int count = 5;
	char items[5][20];

	if (use_SDCARD) sprintf(items[0], "Device: SDCARD");
	else sprintf(items[0], "Device:  MCARD");

	if (CARDSLOT == CARD_SLOTA) sprintf(items[1], "Use: SLOT A");
    else sprintf(items[1], "Use: SLOT B");

	sprintf(items[2], "Load WRAM");
	sprintf(items[3], "Save WRAM");
	sprintf(items[4], "Return to previous");

	menu = 2;

	while (quit == 0)
	{
		ret = DoMenu (&items[0], count);
		switch (ret)
		{
			case -1:
			case  4:
				quit = 1;
				break;

			case 0:
				use_SDCARD ^= 1;
				if (use_SDCARD) sprintf(items[0], "Device: SDCARD");
				else sprintf(items[0], "Device:  MCARD");
				break;
			case 1:
				CARDSLOT ^= 1;
				if (CARDSLOT == CARD_SLOTA) sprintf(items[1], "Use: SLOT A");
				else sprintf(items[1], "Use: SLOT B");
				break;
			case 2:
			case 3:
				if (doStateSaveLoad(ret-2)) return 1;
				break;
		}

	}

	menu = prevmenu;
	return 0;
}
				
/****************************************************************************
 * Load ROM Menu
 ****************************************************************************/
extern void OpenDVD ();
extern void OpenSD ();
extern u8 UseSDCARD;

void loadmenu ()
{
	int prevmenu = menu;
	int ret;
	int quit = 0;
	int count = 3;
	char item[3][20] = {
		{"Load from DVD"},
		{"Load from SDCARD"},
		{"Return to previous"}
	};

	menu = UseSDCARD ? 1 : 0;
	
	while (quit == 0)
	{
		ret = DoMenu (&item[0], count);
		switch (ret)
		{
			case -1: /*** Button B ***/
			case 2:  /*** Quit ***/
				quit = 1;
				menu = prevmenu;
				break;
			case 0:	 /*** Load from DVD ***/
				OpenDVD ();
				quit = 1;
				break;
			case 1:  /*** Load from SCDARD ***/
				OpenSD ();
				quit = 1;
				break;
		}
	}
}


/****************************************************************************
 * Main Menu
 *
 ****************************************************************************/
extern void ResetPCE();
extern void ResetSound();
extern unsigned int timer_60;
extern int hugoromsize;
extern unsigned char cart_reload;

unsigned int savetimer;
int gamepaused = 0;

void MainMenu()
{
	int *psoid = (int *) 0x80001800;
    void (*PSOReload) () = (void (*)()) 0x80001800;
	int ret;
	int quit = 0;
	int count = 7;
	char item[7][20] = {
		"Play Game",  
		"Reset Game", 
		"Load New Game",
		"WRAM Management", 
		"Stop DVD Motor",
		"System Reboot",
		"View Credits"
	};

	menu = 0;
	savetimer = timer_60;
	gamepaused = 1;
	while(copynow == GX_TRUE ) usleep(50);
	
	while (quit == 0)
	{
		ret = DoMenu (&item[0], count);

		switch (ret)
		{
			case -1:
			case  0: /*** Play Game ***/
				quit = 1;
				break;
			case 1:  /*** Reset Emulator */
				if (!cart_reload && hugoromsize)
				{
					ResetPCE();
					ResetSound();
					savetimer = 0;
					quit = 1;
				}
				break;
			case 2 : /*** Load a new game ***/
				loadmenu();
				menu = 0;
				break;	
   			case 3 : /*** WRAM Manager ***/
				quit = wrammenu();
				break;
			case 4 :
				ShowAction ("Stopping DVD Motor ...");
				dvd_motor_off();
				break;
			case 5 : /*** PSO / SD Reload ***/
				if (psoid[0] == PSOSDLOADID) PSOReload();
				else SYS_ResetSystem(SYS_HOTRESET,0,FALSE);
				break;
			case 6 : /*** View Credits ***/
				credits();
				while ( PAD_ButtonsDown(0) & PAD_BUTTON_A );
				break;
		}
	}

	while(PAD_ButtonsHeld(0)) VIDEO_WaitVSync();
	uselessinquiry ();		/*** Stop the DVD from causing clicks while playing ***/
	gamepaused = 0;
	timer_60 = savetimer;
}


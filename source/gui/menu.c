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
	while (!(PAD_ButtonsDown(0) & PAD_BUTTON_A));
	while (PAD_ButtonsDown(0) & PAD_BUTTON_A);
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
 * OPTION Menu
 ****************************************************************************/
extern u8 use_480i;
extern u8 aspect;
extern GXRModeObj *tvmodes[2];
int optionmenu ()
{
	int prevmenu = menu;
	int quit = 0;
	int ret;
	int count = 3;
	char items[3][20];

	menu = 2;

	while (quit == 0)
	{
		sprintf(items[0], "Aspect: %s", aspect ? "ORIGINAL" : "STRETCH");
		if (use_480i == 1) sprintf (items[1], "Render: BILINEAR");
		else if (use_480i == 2) sprintf (items[1], "Render: PROGRESS");
		else sprintf (items[1], "Render: ORIGINAL");
		sprintf(items[2], "Return to previous");

		ret = DoMenu (&items[0], count);
		switch (ret)
		{
			case -1:
			case  2:
				quit = 1;
				break;

			case 0:
				aspect ^= 1;
				osd_gfx_init_normal_mode();
				break;
			case 1:
#ifdef FORCE_EURGB60
				use_480i ^= 1;
#else
				use_480i = (use_480i + 1) % 3;
				if (use_480i == 2)
				{
					/* progressive mode (60hz only) */
					tvmodes[1]->viTVMode |= VI_PROGRESSIVE;
					tvmodes[1]->xfbMode = VI_XFBMODE_SF;
				}
				else
				{
					/* reset video mode */
					tvmodes[1]->viTVMode &= ~VI_PROGRESSIVE;
					tvmodes[1]->xfbMode = VI_XFBMODE_DF;
				}
#endif
				osd_gfx_init_normal_mode();
				break;
		}

	}

	menu = prevmenu;
	return 0;
}
			
/****************************************************************************
 * Load ROM Menu
 ****************************************************************************/
/****************************************************************************
 * Load game Menu
 *
 ****************************************************************************/
extern void OpenDVD ();
extern int OpenSD (u8 slot);
extern u8 UseSDCARD;

static u8 load_menu = 0;
void loadmenu ()
{
  int quit = 0;
  int ret;
  int loadcount = 4;
  char loadmenu[4][20] =
  {
#ifdef HW_RVL
    {"Load from FRONT SD"},
#else
    {"Load from DVD"},
#endif
    {"Load from SD SLOTA"},
    {"Load from SD SLOTB"},
    {"Return to previous"}
  };

  menu = load_menu;

  while (quit == 0)
  {
    ret = DoMenu(&loadmenu[0], loadcount);

    switch (ret)
    {
      case -1:
      case  3:
        quit = 1;
        break;
      
      case 0:
#ifndef HW_RVL
        OpenDVD ();
#else
        OpenSD (2);
#endif
        quit = 1;
        break;
      
      case 1:  /*** Load from SCDARD ***/
      case 2: 
        OpenSD (ret - 1);
        quit = 1;
    }
  }
  
  load_menu = menu;
}


/****************************************************************************
 * Main Menu
 *
 ****************************************************************************/
#ifndef HW_RVL
#endif


extern void ResetPCE();
extern void ResetSound();
extern unsigned int timer_60;
extern int hugoromsize;
extern unsigned char cart_reload;

unsigned int savetimer;
int gamepaused = 0;

void MainMenu()
{
	GXRModeObj *rmode;
    Mtx p;
    menu = 0;
	int ret;
	int quit = 0;
#if defined(HW_RVL)
	void (*TPreload)() = (void(*)())0x90000020;
	int count = 7;
	char items[7][20] =
#else
	int *psoid = (int *) 0x80001800;
	void (*PSOReload) () = (void (*)()) 0x80001800;
	int count = 8;
	char items[8][20] =
#endif
	{
		{"Play Game"},
		{"Hard Reset"},
		{"Load New Game"},
		{"Emulator Options"},
		{"WRAM Management"}, 
#ifdef HW_RVL
		{"TP/Hack Reload"},
#else
		{"Stop DVD Motor"},
		{"SD/PSO Reload"},
#endif
		{"System Reboot"}
	};

	savetimer = timer_60;
	gamepaused = 1;
	while(copynow == GX_TRUE ) usleep(50);

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

   			case 3 : /*** Options ***/
				optionmenu();
				break;

   			case 4 : /*** WRAM Manager ***/
				quit = wrammenu();
				break;

#ifdef HW_RVL
			case 5 :
				TPreload();
				break;

			case 6:
				SYS_ResetSystem(SYS_RESTART, 0, 0);
				break;
#else
			case 5:
				ShowAction("Stopping DVD Motor ...");
				dvd_motor_off();
				break;

			case 6:
				if (psoid[0] == PSOSDLOADID) PSOReload ();
				break;

			case 7:
				SYS_ResetSystem(SYS_HOTRESET,0,0);
				break;
#endif
		}
	}

	while(PAD_ButtonsHeld(0)) VIDEO_WaitVSync();

	/*** Reinitialize current TV mode ***/
	rmode = tvmodes[use_480i];
	VIDEO_Configure (rmode);
	VIDEO_ClearFrameBuffer(rmode, xfb[whichfb], COLOR_BLACK);
	VIDEO_Flush();
	VIDEO_WaitVSync();
    VIDEO_WaitVSync();

	/*** Reinitalize GX ***/ 
    GX_SetViewport (0.0F, 0.0F, rmode->fbWidth, rmode->efbHeight, 0.0F, 1.0F);
	GX_SetScissor (0, 0, rmode->fbWidth, rmode->efbHeight);
    f32 yScale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    u16 xfbHeight  = GX_SetDispCopyYScale (yScale);
	GX_SetDispCopySrc (0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst (rmode->fbWidth, xfbHeight);
	GX_SetCopyFilter (rmode->aa, rmode->sample_pattern, use_480i ? GX_TRUE : GX_FALSE, rmode->vfilter);
	GX_SetFieldMode (rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt (rmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	guOrtho(p, rmode->efbHeight/2, -(rmode->efbHeight/2), -(rmode->fbWidth/2), rmode->fbWidth/2, 100, 1000);
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);
	GX_Flush();

    gamepaused = 0;
	timer_60 = savetimer;

#ifndef HW_RVL
  /*** Stop the DVD from causing clicks while playing ***/
  uselessinquiry ();
#endif
}

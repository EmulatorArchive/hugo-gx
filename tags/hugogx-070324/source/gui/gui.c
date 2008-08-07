/****************************************************************************
 * Nintendo Gamecube User Menu
 ****************************************************************************/
#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"
#include "hugologo.h"
#include "iplfont.h"
#define PSOSDLOADID 0x7c6000a6

extern unsigned int *xfb[2];
extern int whichfb;
extern GXRModeObj *vmode;
extern int font_height;
extern void ResetPCE();
extern void ResetSound();
extern int doStateSaveLoad( int which , int card );
extern unsigned int timer_60;
unsigned int *backdrop;
unsigned int savetimer;
int gamepaused = 0;
char version[] = { "Version 2.12" };
extern int OpenDVD();
extern int OpenSD();
extern void dvd_motor_off();
unsigned int backcolour;
extern int copynow;
extern void uselessinquiry();

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
	temp = malloc( outbytes + 16 );
	res = uncompress( (char *)temp, &outbytes, (char *)hugologo, inbytes );

	memcpy(&w, temp, 4);
	backcolour = w;

	/*** Fill backdrop ***/
	for ( h = 0; h < ( 320 * 480 ); h++ )
		backdrop[h] = w;

	/*** Put picture in position ***/
	v = 0;
	for ( h = 0; h < hugologo_HEIGHT; h++ )
	{
		for( w = 0; w < hugologo_WIDTH >> 1; w++ )
			backdrop[ ( ( h + 40 ) * 320 ) + w + 89 ] = temp[v++];		
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

	for ( i = 0; i < 100; i++ )
	{
		whichfb ^= 1;
		for ( h = 0; h < ( 320 * 480 ); h++ )
			xfb[whichfb][h] = w;

		/*** Now pour 4 lines at a time ***/
		/*** Copy base on screen ***/
		if ( linecount ) {
			memcpy(&xfb[whichfb][((334 - linecount) * 320)], 
			       backdrop +((228 - linecount) * 320),
			       linecount * 4 * 320);

			for ( v = 0; v < (334-linecount); v++ )
				memcpy(&xfb[whichfb][v * 320], backdrop + ((228-linecount) * 320), 320 * 4);
		}

		linecount += 2;

		write_centre(340, version);
		ShowScreen();
	}

	for ( i = 0; i < 200; i++ )
		VIDEO_WaitVSync();
	
}

/****************************************************************************
 * Credits
 ****************************************************************************/
void credits()
{
	int p;

	p = 220 + ( font_height << 1);
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
 * DrawMenu
 ****************************************************************************/
void DrawMenu( char items[][20], int maxitems, int selected )
{
	int i,p;

        copybackdrop();

        /*** Put title ***/
        write_centre( 230, version);

	p = 230 + (font_height << 1);

	for ( i = 0 ; i < maxitems ; i++ )
	{
		if ( i == selected )
			write_centre_hi( p, items[i] );
		else
			write_centre( p, items[i] );

		p += font_height;
	}

	ShowScreen();
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
	while ( !(PAD_ButtonsDown(0) & PAD_BUTTON_A ) );
	while ( PAD_ButtonsDown(0) & PAD_BUTTON_A );
}

void ShowAction( char *prompt )
{
	copybackdrop();
	write_centre( 290, prompt);
	ShowScreen();
}

void ClearScreen()
{
	whichfb ^= 1;
	VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], backcolour);
}

/****************************************************************************
 * Emulator Options
 ****************************************************************************/
int emucount = 6;
char emuoptions[6][20] = { "Reset Emulator", 
			   "Load WRAM SLOT A",
			   "Load WRAM SLOT B", 
   			   "Save WRAM SLOT A",
			   "Save WRAM SLOT B",
			   "Return to Main" };
void EmuOptions()
{

	int menu = 0;
	int redraw = 1;
	int quit = 0;
	short j;
	
	while( !( PAD_ButtonsDown(0) & PAD_BUTTON_B ) && ( quit == 0 ))
	{

		if ( redraw ) {
			DrawMenu(&emuoptions[0], emucount, menu);
			redraw = 0;
		}

		j = PAD_ButtonsDown(0);

		if ( j & PAD_BUTTON_UP )
		{	menu--;
			redraw = 1;
		}

		if ( j & PAD_BUTTON_DOWN )
		{	menu++;
			redraw = 1;
		}

		if ( j & PAD_BUTTON_A )
		{
			redraw = 1;
			switch( menu )
			{

				case 0:	/*** Reset ***/
					ResetPCE();
					ResetSound();
					savetimer = 0;
					break;

				case 1:	/*** Load SLOT A ***/
					doStateSaveLoad( 0, CARD_SLOTA );
					break;

				case 2: /*** Load SLOT B ***/
					doStateSaveLoad( 0, CARD_SLOTB );
					break;

				case 3: /*** Save SLOT A ***/
					doStateSaveLoad( 1, CARD_SLOTA );
					break;

				case 4:	/*** Save SLOT B ***/
					doStateSaveLoad( 1, CARD_SLOTB );
					break;

				case 5: /*** Return ***/
					quit = 1;
					break;
			}

			while( PAD_ButtonsDown(0) & PAD_BUTTON_A );
		}

		if ( menu > emucount )
			menu = 0;

		if ( menu < 0 )
			menu = emucount - 1;

	}

	/*** Remove any exhausted keys ***/
	while ( PAD_ButtonsDown(0) && PAD_BUTTON_B );
}

/****************************************************************************
 * Main Menu
 *
 * 1) Play Game
 * 2) Reset Emulator
 * 3) Save Manager
 * 4) Credits
 ****************************************************************************/
int maincount = 7;
char mainmenu[7][20] = { "Play Game",  
			 "Emulator Options", 
			 "Load From DVD",
			 "Stop DVD Motor",
			 "Load From SD Card",
			 "SD/PSO Reload",
			 "View Credits"
 };

void MainMenu()
{
	int menu = 0;
	int redraw = 1;
	int quit = 0;
	short j;
	int *psoid = (int *) 0x80001800;
    void (*PSOReload) () = (void (*)()) 0x80001800;

	savetimer = timer_60;
	gamepaused = 1;

	while(copynow == GX_TRUE ) usleep(50);
	
	while ( !( PAD_ButtonsDown(0) & PAD_BUTTON_B ) && ( quit == 0 ) )
	{
		if ( redraw ) {
			DrawMenu( &mainmenu[0], maincount, menu );
			redraw = 0;
		}

		j = PAD_ButtonsDown(0);

		if ( j & PAD_BUTTON_UP ) {
			menu--;
			redraw = 1;
		}

		if ( j & PAD_BUTTON_DOWN ) {
			menu++;
			redraw = 1;
		}

		if ( j & PAD_BUTTON_A ) {
		
			redraw = 1;
	
			switch( menu )
			{
				case 0 : /*** Play Game ***/
					quit = 1;
					break;
     			case 1 : /*** Emulator Options ***/
					EmuOptions();
					break;
				case 2 : /*** Load NEW ROM ***/
					OpenDVD();
					menu = 0;
					break;
				case 3 :
					dvd_motor_off();
					break;
				case 4 : /*** Load from SD-card ***/
					OpenSD();
					menu = 0;
					break;	
				case 5 : /*** PSO / SD Reload ***/
					if (psoid[0] == PSOSDLOADID) PSOReload();
					break;
				case 6 : /*** View Credits ***/
					credits();
					while ( PAD_ButtonsDown(0) & PAD_BUTTON_A );
					break;
			}
			
			while( PAD_ButtonsDown(0) & PAD_BUTTON_A );
		}

		if (menu == maincount) menu = 0;
		if (menu < 0) menu = maincount - 1;
	}
	uselessinquiry ();		/*** Stop the DVD from causing clicks while playing ***/

	gamepaused = 0;
	timer_60 = savetimer;
}


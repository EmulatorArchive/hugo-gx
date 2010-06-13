/****************************************************************************
 * Nintendo Gamecube User Menu
 ****************************************************************************/
#include "pce.h"
#include "osd_ngc_machine.h"
#include "font.h"
#include "hugologo.h"
#include "filesel.h"
#include "file_dvd.h"
#include "file_fat.h"
#include "dvd.h"

#include <network.h>
#include <zlib.h>


#ifdef HW_RVL
#include "preferences.h"
#include <wiiuse/wpad.h>
#include <di/di.h>
#include <smb.h>

struct SHugoSettings HugoSettings;
#endif

extern unsigned int *xfb[2];
extern int whichfb;
extern GXRModeObj *vmode;
extern int copynow;
extern void ogc_video__reset();
extern void ResetSound();
extern int hugoromsize;
int networkInit;
unsigned int savetimer;
unsigned int *backdrop;
unsigned int backcolour;
char version[] = { "Version 2.12" };


int smbmenu();



	

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

static void copybackdrop()
{
  whichfb ^= 1;
  memcpy (xfb[whichfb], backdrop,  320 * 480 * 4);
}

/****************************************************************************
 *  Display functions
 *
 ****************************************************************************/
u8 SILENT = 0;

void SetScreen ()
{
  VIDEO_SetNextFramebuffer (xfb[whichfb]);
  VIDEO_Flush ();
  VIDEO_WaitVSync ();
}

void ClearScreen()
{
  whichfb ^= 1;
  VIDEO_ClearFrameBuffer(vmode, xfb[whichfb], backcolour);
}

void WaitButtonA ()
{
  s16 p = ogc_input__getMenuButtons();
  while (p & PAD_BUTTON_A)    p = ogc_input__getMenuButtons();
  while (!(p & PAD_BUTTON_A)) p = ogc_input__getMenuButtons();
}

extern void WaitPrompt (char *msg)
{
  if (SILENT) return;
  ClearScreen();
  WriteCentre(254, msg);
  WriteCentre(254 + fheight, "Press A to Continue");
  SetScreen();
  WaitButtonA ();
}

extern void ShowAction (char *msg)
{
  if (SILENT) return;

  ClearScreen();
  WriteCentre(254, msg);
  SetScreen();
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
    WriteCentre(340, version);
    SetScreen();
  }

  sleep(1);
}

/****************************************************************************
 * Credits
 ****************************************************************************/
void credits()
{
#ifdef HW_RVL
  load_settings();
#endif
  int p = 220 + ( fheight << 1);
  copybackdrop();
  WriteCentre(p, "Hu-Go! - Zeograd http://www.zeograd.com");
  p += fheight;
  WriteCentre(p, "PCE PSG Info - Paul Clifford / John Kortink / Ki");
  p += fheight;
  WriteCentre(p, "Gamecube & Wii Port - softdev / eke-eke");
  p += fheight;
  WriteCentre(p, "libOGC - shagkur / wntrmute");
  p += fheight;
  WriteCentre(p, "Support - http://www.tehskeen.com");
  p += ( fheight << 1 );
  WriteCentre(p, "Press Button A");
  SetScreen();
  WaitButtonA ();
}

/****************************************************************************
 * Menu
 ****************************************************************************/
static int menu = 0;

void DrawMenu( char items[][20], int maxitems, int selected )
{
  int i,p;

  copybackdrop();
  WriteCentre( 210, version);
  p = 210 + (fheight << 1);
  for ( i = 0 ; i < maxitems ; i++ )
  {
    if ( i == selected ) WriteCentre_HL( p, items[i] );
    else WriteCentre( p, items[i] );
    p += fheight;
  }
  SetScreen();
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

  sprintf(items[1], "Save WRAM");
  sprintf(items[2], "Load WRAM");
  sprintf(items[3], "Return to previous");

  menu = 2;

  while (quit == 0)
  {
    if (device == 0) sprintf(items[0], "Device: FAT");
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
  int count;
#ifdef HW_RVL  
  char items[4][20];
  count = 4;
#else
  char items[3][20];
  count = 3;
#endif
  //}
  
  menu = 0;

  while (quit == 0)
  {
    sprintf(items[0], "Aspect: %s", aspect ? "ORIGINAL" : "STRETCH");
    if (render == 1) sprintf (items[1], "Render: INTERLACED");
    else if (render == 2) sprintf (items[1], "Render: PROGRESSIVE");
    else sprintf (items[1], "Render: ORIGINAL");
#ifdef HW_RVL  
    sprintf (items[2],"SMB Settings");
	sprintf (items[3],"Return to previous");
#else
	sprintf (items[2],"Return to previous");
#endif

    ret = DoMenu (&items[0], count);
    switch (ret)
    {

#ifndef HW_RVL  
	  case 2:
#else
	  case 2:
		smbmenu();
#endif
      case -1:
	  case 3:
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
        break;

    }
  }

  menu = prevmenu;
  return 0;
}

/****************************************************************************
 * SMB Settings Menu
 ****************************************************************************/
#ifdef HW_RVL

void initNetwork()
{
	ShowAction("initializing network...");
	while (net_init() == -EAGAIN);
	char myIP[16];

	if (if_config(myIP, NULL, NULL, true) < 0) 
	{
		WaitPrompt("failed to init network interface\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		networkInit = 1;
		if ((strlen(HugoSettings.share)) > 0 && (strlen(HugoSettings.ip) > 0))
		{
			if (!smbInit(HugoSettings.user, HugoSettings.pwd, HugoSettings.share, HugoSettings.ip)) 
			{
				printf("failed to connect to SMB share\n");
				exit(EXIT_FAILURE);
			}
		}
		else
			WaitPrompt("Wrong Parameters - Check your Settings.xml");
	}
	
}

void closeNetwork()
{
	if(networkInit)
		smbClose("smb");
	networkInit = false;
	
} 

 
int smbmenu ()
{
  int prevmenu = menu;
  int quit = 0;
  int ret;
  int count = 5;
  char items[5][20];

  menu = 1;

  while (quit == 0)
  {
    sprintf (items[0], "IP: %s", HugoSettings.ip);
    sprintf (items[1], "Share: %s", HugoSettings.share);
	sprintf (items[2], "Username: %s", HugoSettings.user);
    sprintf (items[3], "Password: %s", HugoSettings.pwd);	
	sprintf (items[4],"Return to previous");


    ret = DoMenu (&items[0], count);
    switch (ret)
    {
      case -1:
	  case 4:
	    quit = 1;
        break;

      case 0:
	  // open OnScreenKeyboard for user Settings in future versions...
      case 1:
	  case 2:
	  case 3:	  
        break;

    }
  }
  menu = prevmenu;
  return 0;
}
#endif
/****************************************************************************
 * Load Rom menu
 *
 ****************************************************************************/
extern int hugoromsize;
extern unsigned char *hugorom;
extern char rom_filename[MAXJOLIET];
static u8 load_menu = 0;
static u8 dvd_on = 0;


int loadmenu ()
{
  int prevmenu = menu;
  int ret,count,size;
  int quit = 0;
#ifdef HW_RVL
  char item[6][20] = {
    {"Load Recent"},
    {"Load from SD"},
    {"Load from USB"},
    {"Load from DVD"},
	{"Load from SMB"},
    {"Stop DVD Motor"}
  };
#else
  char item[5][20] = {
    {"Load Recent"},
    {"Load from SD"},
    {"Load from DVD"},
    {"Stop DVD Motor"}	
  };
#endif

  menu = load_menu;
  
  while (quit == 0)
  {
#ifdef HW_RVL
    count = 5 + dvd_on;
#else
    count = 3 + dvd_on;
#endif
    ret = DoMenu (&item[0], count);
    switch (ret)
    {
      /*** Button B ***/
      case -1: 
        quit = 1;
        break;

      /*** Load from DVD ***/
#ifdef HW_RVL
      

      /*** Load from SMB Share ***/
	  case 4:
		if (!networkInit)
			initNetwork();
			
	    load_menu = menu;
        size = FAT_Open(ret,hugorom);
        if (size)
        {
		   hugoromsize = size;
		   cart_reload = 1;
		   sprintf(rom_filename,"%s",filelist[selection].filename);
		   rom_filename[strlen(rom_filename) - 4] = 0;
		   return 1;
        }
        break;
	  
	  case 3:
#else
      case 2:
#endif
        load_menu = menu;
        size = DVD_Open(hugorom);
        if (size)
        {
          dvd_on = 1;
          hugoromsize = size;
          cart_reload = 1;
          sprintf(rom_filename,"%s",filelist[selection].filename);
          rom_filename[strlen(rom_filename) - 4] = 0;
          return 1;
        }
        break;

      /*** Stop DVD Disc ***/
#ifdef HW_RVL
      case 5:  
#else
      case 3:
#endif
        dvd_motor_off();
        dvd_on = 0;
        menu = load_menu;
        break;

      /*** Load from FAT device ***/
      default:
        load_menu = menu;
        size = FAT_Open(ret,hugorom);
        if (size)
        {
          hugoromsize = size;
          cart_reload = 1;
          sprintf(rom_filename,"%s",filelist[selection].filename);
          rom_filename[strlen(rom_filename) - 4] = 0;
          return 1;
        }
        break;
    }
  }

  menu = prevmenu;
  return 0;
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
        quit = loadmenu();
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

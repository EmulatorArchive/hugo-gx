/******************************************************************************
 *
 *  Hu-Go!
 *  GC/Wii Input specific
 *  
 ***************************************************************************/

#include <hard_pce.h>
#include <pce.h>
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"

#ifdef HW_RVL
#include <wiiuse/wpad.h>
#endif

#define MAX_INPUTS 4

/* configurable keys */
#define KEY_BUTTON_I      0   
#define KEY_BUTTON_II     1
#define KEY_SELECT        2
#define KEY_RUN           3
#define KEY_AUTOFIRE_I    4
#define KEY_AUTOFIRE_II   5
#define KEY_MENU          6
#define MAX_KEYS          7

/* Hu-Go keys */
#define	JOY_A		    0x01
#define	JOY_B		    0x02
#define	JOY_SELECT	0x04
#define	JOY_RUN	    0x08
#define	JOY_UP		  0x10
#define	JOY_RIGHT	  0x20
#define	JOY_DOWN	  0x40
#define	JOY_LEFT	  0x80

/* gamepad default map (this can be reconfigured) */
u16 pad_keymap[MAX_INPUTS][MAX_KEYS] =
{
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_TRIGGER_Z, PAD_BUTTON_START, PAD_BUTTON_X, PAD_BUTTON_Y, PAD_TRIGGER_L},
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_TRIGGER_Z, PAD_BUTTON_START, PAD_BUTTON_X, PAD_BUTTON_Y, PAD_TRIGGER_L},
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_TRIGGER_Z, PAD_BUTTON_START, PAD_BUTTON_X, PAD_BUTTON_Y, PAD_TRIGGER_L},
  {PAD_BUTTON_B, PAD_BUTTON_A, PAD_TRIGGER_Z, PAD_BUTTON_START, PAD_BUTTON_X, PAD_BUTTON_Y, PAD_TRIGGER_L}
};

/* gamepad available buttons */
static u16 pad_keys[8] =
{
  PAD_TRIGGER_Z,
  PAD_TRIGGER_R,
  PAD_TRIGGER_L,
  PAD_BUTTON_A,
  PAD_BUTTON_B,
  PAD_BUTTON_X,
  PAD_BUTTON_Y,
  PAD_BUTTON_START,
};

#ifdef HW_RVL

/* wiimote default map (this can be reconfigured) */
u32 wpad_keymap[MAX_INPUTS*3][MAX_KEYS] =
{
  /* Wiimote #1 */
  {WPAD_BUTTON_1, WPAD_BUTTON_2, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_HOME},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_NUNCHUK_BUTTON_Z, WPAD_NUNCHUK_BUTTON_C, WPAD_BUTTON_HOME},
  {WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS, WPAD_CLASSIC_BUTTON_MINUS, WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_HOME},
  
  /* Wiimote #2 */
  {WPAD_BUTTON_1, WPAD_BUTTON_2, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_HOME},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_NUNCHUK_BUTTON_Z, WPAD_NUNCHUK_BUTTON_C, WPAD_BUTTON_HOME},
  {WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS, WPAD_CLASSIC_BUTTON_MINUS, WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_HOME},

  /* Wiimote #3 */
  {WPAD_BUTTON_1, WPAD_BUTTON_2, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_HOME},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_NUNCHUK_BUTTON_Z, WPAD_NUNCHUK_BUTTON_C, WPAD_BUTTON_HOME},
  {WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS, WPAD_CLASSIC_BUTTON_MINUS, WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_HOME},

  /* Wiimote #4 */
  {WPAD_BUTTON_1, WPAD_BUTTON_2, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_HOME},
  {WPAD_BUTTON_A, WPAD_BUTTON_B, WPAD_BUTTON_PLUS, WPAD_BUTTON_MINUS, WPAD_NUNCHUK_BUTTON_Z, WPAD_NUNCHUK_BUTTON_C, WPAD_BUTTON_HOME},
  {WPAD_CLASSIC_BUTTON_B, WPAD_CLASSIC_BUTTON_A, WPAD_CLASSIC_BUTTON_PLUS, WPAD_CLASSIC_BUTTON_MINUS, WPAD_CLASSIC_BUTTON_Y, WPAD_CLASSIC_BUTTON_X, WPAD_CLASSIC_BUTTON_HOME},
};

/* directional buttons default mapping (this can NOT be reconfigured) */
#define PAD_UP    0   
#define PAD_DOWN  1
#define PAD_LEFT  2
#define PAD_RIGHT 3

static u32 wpad_dirmap[3][4] =
{
  {WPAD_BUTTON_RIGHT, WPAD_BUTTON_LEFT, WPAD_BUTTON_UP, WPAD_BUTTON_DOWN},                                // WIIMOTE only
  {WPAD_BUTTON_UP, WPAD_BUTTON_DOWN, WPAD_BUTTON_LEFT, WPAD_BUTTON_RIGHT},                                // WIIMOTE + NUNCHUK
  {WPAD_CLASSIC_BUTTON_UP, WPAD_CLASSIC_BUTTON_DOWN, WPAD_CLASSIC_BUTTON_LEFT, WPAD_CLASSIC_BUTTON_RIGHT} // CLASSIC
};

/* wiimote/expansion available buttons */
static u32 wpad_keys[20] =
{
  WPAD_BUTTON_2,
  WPAD_BUTTON_1,
  WPAD_BUTTON_B,
  WPAD_BUTTON_A,
  WPAD_BUTTON_MINUS,
  WPAD_BUTTON_HOME,
  WPAD_BUTTON_PLUS,
  WPAD_NUNCHUK_BUTTON_Z,
  WPAD_NUNCHUK_BUTTON_C,
  WPAD_CLASSIC_BUTTON_ZR,
  WPAD_CLASSIC_BUTTON_X,
  WPAD_CLASSIC_BUTTON_A,
  WPAD_CLASSIC_BUTTON_Y,
  WPAD_CLASSIC_BUTTON_B,
  WPAD_CLASSIC_BUTTON_ZL,
  WPAD_CLASSIC_BUTTON_FULL_R,
  WPAD_CLASSIC_BUTTON_PLUS,
  WPAD_CLASSIC_BUTTON_HOME,
  WPAD_CLASSIC_BUTTON_MINUS,
  WPAD_CLASSIC_BUTTON_FULL_L,
};
#endif

static const char *keys_name[MAX_KEYS] =
{
  "Button I",
  "Button II",
  "Button SELECT",
  "Button RUN",
  "Autofire I",
  "Autofire II",
  "MENU"
};

static int autofire[2] = { 0, 0 };
static int autofireon = 0;

extern void WaitPrompt(char *prompt);
extern void ShowAction(char *prompt);
extern void ShowScreen();
extern void ClearScreen();
extern void MainMenu();

/*******************************
  gamepad support
*******************************/
static void pad_config(int num)
{
  int i,j;
  u16 p;
  u8 quit;
  char msg[30];

  u32 connected = PAD_ScanPads();
  if ((connected & (1<<num)) == 0)
  {
    WaitPrompt("PAD is not connected !");
    return;
  }

  /* configure keys */
  for (i=0; i<MAX_KEYS; i++)
  {
    /* remove any pending keys */
    while (PAD_ButtonsHeld(num))
    {
      VIDEO_WaitVSync();
      PAD_ScanPads();
    }

    ClearScreen();
    sprintf(msg,"Press key for %s",keys_name[i]);
    write_centre(254, msg);
    ShowScreen();

    /* check buttons state */
    quit = 0;
    while (quit == 0)
    {
      VIDEO_WaitVSync();
      PAD_ScanPads();
      p = PAD_ButtonsDown(num);

      for (j=0; j<8; j++)
      {
        if (p & pad_keys[j])
        {
           pad_keymap[num][i] = pad_keys[j];
           quit = 1;
           j = 9;   /* exit loop */
        }
      }
    }
  }
}

static void pad_update()
{
  int i;
  u16 p;
	s8 x,y;

  /* update PAD status */
  PAD_ScanPads();

  for (i=0; i<MAX_INPUTS; i++)
  {
    x = PAD_StickX (i);
    y = PAD_StickY (i);
    p = PAD_ButtonsHeld(i);

    if ((p & PAD_BUTTON_UP)    || (y >  70)) io.JOY[i] |= JOY_UP;
    else if ((p & PAD_BUTTON_DOWN)  || (y < -70)) io.JOY[i] |= JOY_DOWN;
    if ((p & PAD_BUTTON_LEFT)  || (x < -60)) io.JOY[i] |= JOY_LEFT;
    else if ((p & PAD_BUTTON_RIGHT) || (x >  60)) io.JOY[i] |= JOY_RIGHT;

    /* MENU */
    if (p & pad_keymap[i][KEY_MENU]) MainMenu();

    /* BUTTONS */
    if (p & pad_keymap[i][KEY_BUTTON_I])   io.JOY[i]  |= JOY_A;
    if (p & pad_keymap[i][KEY_BUTTON_II])  io.JOY[i]  |= JOY_B;
    if (p & pad_keymap[i][KEY_SELECT])     io.JOY[i]  |= JOY_SELECT;
    if (p & pad_keymap[i][KEY_RUN])        io.JOY[i]  |= JOY_RUN;

    /* AUTOFIRE */
    if (p & pad_keymap[i][KEY_AUTOFIRE_I])   autofire[0] ^= 1;
    if (p & pad_keymap[i][KEY_AUTOFIRE_II])  autofire[1] ^= 1;
    if (autofireon)
    {
      if (autofire[0]) io.JOY[i] |= JOY_A;
      if (autofire[1]) io.JOY[i] |= JOY_B;
    }
  }
}

/*******************************
  wiimote support
*******************************/
#ifdef HW_RVL
#define PI 3.14159265f

static s8 WPAD_StickX(u8 chan,u8 right)
{
  float mag = 0.0;
  float ang = 0.0;
  WPADData *data = WPAD_Data(chan);

  switch (data->exp.type)
  {
    case WPAD_EXP_NUNCHUK:
    case WPAD_EXP_GUITARHERO3:
      if (right == 0)
      {
        mag = data->exp.nunchuk.js.mag;
        ang = data->exp.nunchuk.js.ang;
      }
      break;

    case WPAD_EXP_CLASSIC:
      if (right == 0)
      {
        mag = data->exp.classic.ljs.mag;
        ang = data->exp.classic.ljs.ang;
      }
      else
      {
        mag = data->exp.classic.rjs.mag;
        ang = data->exp.classic.rjs.ang;
      }
      break;

    default:
      break;
  }

  /* calculate X value (angle need to be converted into radian) */
  if (mag > 1.0) mag = 1.0;
  else if (mag < -1.0) mag = -1.0;
  double val = mag * sin(PI * ang/180.0f);
 
  return (s8)(val * 128.0f);
}


static s8 WPAD_StickY(u8 chan, u8 right)
{
  float mag = 0.0;
  float ang = 0.0;
  WPADData *data = WPAD_Data(chan);

  switch (data->exp.type)
  {
    case WPAD_EXP_NUNCHUK:
    case WPAD_EXP_GUITARHERO3:
      if (right == 0)
      {
        mag = data->exp.nunchuk.js.mag;
        ang = data->exp.nunchuk.js.ang;
      }
      break;

    case WPAD_EXP_CLASSIC:
      if (right == 0)
      {
        mag = data->exp.classic.ljs.mag;
        ang = data->exp.classic.ljs.ang;
      }
      else
      {
        mag = data->exp.classic.rjs.mag;
        ang = data->exp.classic.rjs.ang;
      }
      break;

    default:
      break;
  }

  /* calculate X value (angle need to be converted into radian) */
  if (mag > 1.0) mag = 1.0;
  else if (mag < -1.0) mag = -1.0;
  double val = mag * cos(PI * ang/180.0f);
 
  return (s8)(val * 128.0f);
}

static void wpad_config(u8 pad)
{
  int i,j;
  u8 quit;
  u32 exp;
  char msg[30];

  /* check WPAD status */
  if (WPAD_Probe(pad, &exp) != WPAD_ERR_NONE)
  {
    WaitPrompt("Wiimote is not connected !");
    return;
  }

  /* index for wpad_keymap */
  u8 index = exp + (pad * 3);

  /* loop on each mapped keys */
  for (i=0; i<MAX_KEYS; i++)
  {
    /* remove any pending buttons */
    while (WPAD_ButtonsHeld(pad))
    {
      WPAD_ScanPads();
      VIDEO_WaitVSync();
    }

    /* user information */
    ClearScreen();
    sprintf(msg,"Press key for %s",keys_name[i]);
    write_centre(254, msg);
    ShowScreen();

    /* wait for input */
    quit = 0;
    while (quit == 0)
    {
      WPAD_ScanPads();

      /* get buttons */
      for (j=0; j<20; j++)
      {
        if (WPAD_ButtonsDown(pad) & wpad_keys[j])
        {
          wpad_keymap[index][i]  = wpad_keys[j];
          quit = 1;
          j = 20;    /* leave loop */
        }
      }
    } /* wait for input */ 
  } /* loop for all keys */

  /* removed any pending buttons */
  while (WPAD_ButtonsHeld(pad))
  {
    WPAD_ScanPads();
    VIDEO_WaitVSync();
  }
}

static void wpad_update(void)
{
  int i;
  u32 exp;
  u32 p;
  s8 x,y;

  /* update WPAD data */
  WPAD_ScanPads();

  for (i=0; i<MAX_INPUTS; i++)
  {
    /* check WPAD status */
    if (WPAD_Probe(i, &exp) == WPAD_ERR_NONE)
    {
      p = WPAD_ButtonsHeld(i);
      x = WPAD_StickX(i,0);
      y = WPAD_StickY(i,0);

      if ((p & wpad_dirmap[exp][PAD_UP])          || (y >  70)) io.JOY[i] |= JOY_UP;
      else if ((p & wpad_dirmap[exp][PAD_DOWN])   || (y < -70)) io.JOY[i] |= JOY_DOWN;
      if ((p & wpad_dirmap[exp][PAD_LEFT])        || (x < -60)) io.JOY[i] |= JOY_LEFT;
      else if ((p & wpad_dirmap[exp][PAD_RIGHT])  || (x >  60)) io.JOY[i] |= JOY_RIGHT;

      /* retrieve current key mapping */
      u8 index = exp + (3 * i);

      /* MENU */
      if ((p & wpad_keymap[index][KEY_MENU]) || (p & WPAD_BUTTON_HOME))
        MainMenu();

      /* BUTTONS */
      if (p & wpad_keymap[index][KEY_BUTTON_I])   io.JOY[i]  |= JOY_A;
      if (p & wpad_keymap[index][KEY_BUTTON_II])  io.JOY[i]  |= JOY_B;
      if (p & wpad_keymap[index][KEY_SELECT])     io.JOY[i]  |= JOY_SELECT;
      if (p & wpad_keymap[index][KEY_RUN])        io.JOY[i]  |= JOY_RUN;

      /* AUTOFIRE */
      if (p & wpad_keymap[index][KEY_AUTOFIRE_I])   autofire[0] ^= 1;
      if (p & wpad_keymap[index][KEY_AUTOFIRE_II])  autofire[1] ^= 1;
      if (autofireon)
      {
        if (autofire[0]) io.JOY[i] |= JOY_A;
        if (autofire[1]) io.JOY[i] |= JOY_B;
      }
    }
  }
}

#endif

/*****************************************************************
                Generic input handlers 
******************************************************************/
int osd_init_input()
{
  PAD_Init ();
#ifdef HW_RVL
  WPAD_Init();
	WPAD_SetIdleTimeout(60);
  WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
  WPAD_SetVRes(WPAD_CHAN_ALL,640,480);
#endif

  return 0;
}

void osd_shutdown_input(void)
{
}

int osd_keyboard(void)
{
  int i;
  
  /* reset inputs (five inputs max) */
  for (i=0; i<5; i++) io.JOY[i] = 0;

  /* switch autofire */
  autofireon ^= 1;

  /* update inputs */
  pad_update();
#ifdef HW_RVL
  wpad_update();
#endif

  return cart_reload;
}

void ogc_input__config(u8 pad, u8 type)
{
  switch (type)
  {
    case 0:
      pad_config(pad);
      break;
    
#ifdef HW_RVL
    case 1:
      wpad_config(pad);
      break;
#endif
    
    default:
      break;
  }
}


u16 ogc_input__getMenuButtons(void)
{
  /* gamecube pad */
  PAD_ScanPads();
  u16 p = PAD_ButtonsDown(0);
  s8 x  = PAD_StickX(0);
  s8 y  = PAD_StickY(0);
  if (x > 70) p |= PAD_BUTTON_RIGHT;
  else if (x < -70) p |= PAD_BUTTON_LEFT;
	if (y > 60) p |= PAD_BUTTON_UP;
  else if (y < -60) p |= PAD_BUTTON_DOWN;

#ifdef HW_RVL
  struct ir_t ir;
  u32 exp;
  if (WPAD_Probe(0, &exp) == WPAD_ERR_NONE)
  {
    WPAD_ScanPads();
    u32 q = WPAD_ButtonsDown(0);
    x = WPAD_StickX(0, 0);
    y = WPAD_StickY(0, 0);

    /* default directions */
    WPAD_IR(0, &ir);
    if (ir.valid)
    {
      /* Wiimote is pointed toward screen */
      if ((q & WPAD_BUTTON_UP) || (y > 70))         p |= PAD_BUTTON_UP;
      else if ((q & WPAD_BUTTON_DOWN) || (y < -70)) p |= PAD_BUTTON_DOWN;
      if ((q & WPAD_BUTTON_LEFT) || (x < -60))      p |= PAD_BUTTON_LEFT;
      else if ((q & WPAD_BUTTON_RIGHT) || (x > 60)) p |= PAD_BUTTON_RIGHT;
    }
    else
    {
      /* Wiimote is used horizontally */
      if ((q & WPAD_BUTTON_RIGHT) || (y > 70))         p |= PAD_BUTTON_UP;
      else if ((q & WPAD_BUTTON_LEFT) || (y < -70)) p |= PAD_BUTTON_DOWN;
      if ((q & WPAD_BUTTON_UP) || (x < -60))      p |= PAD_BUTTON_LEFT;
      else if ((q & WPAD_BUTTON_DOWN) || (x > 60)) p |= PAD_BUTTON_RIGHT;
    }

    /* default keys */
    if (q & WPAD_BUTTON_MINUS)  p |= PAD_TRIGGER_L;
    if (q & WPAD_BUTTON_PLUS)   p |= PAD_TRIGGER_R;
    if (q & WPAD_BUTTON_A)      p |= PAD_BUTTON_A;
    if (q & WPAD_BUTTON_B)      p |= PAD_BUTTON_B;
    if (q & WPAD_BUTTON_2)      p |= PAD_BUTTON_A;
    if (q & WPAD_BUTTON_1)      p |= PAD_BUTTON_B;
    if (q & WPAD_BUTTON_HOME)   p |= PAD_TRIGGER_Z;

    /* classic controller expansion */
    if (exp == WPAD_EXP_CLASSIC)
    {
      if (q & WPAD_CLASSIC_BUTTON_UP)         p |= PAD_BUTTON_UP;
      else if (q & WPAD_CLASSIC_BUTTON_DOWN)  p |= PAD_BUTTON_DOWN;
      if (q & WPAD_CLASSIC_BUTTON_LEFT)       p |= PAD_BUTTON_LEFT;
      else if (q & WPAD_CLASSIC_BUTTON_RIGHT) p |= PAD_BUTTON_RIGHT;

      if (q & WPAD_CLASSIC_BUTTON_FULL_L) p |= PAD_TRIGGER_L;
      if (q & WPAD_CLASSIC_BUTTON_FULL_R) p |= PAD_TRIGGER_R;
      if (q & WPAD_CLASSIC_BUTTON_A)      p |= PAD_BUTTON_A;
      if (q & WPAD_CLASSIC_BUTTON_B)      p |= PAD_BUTTON_B;
      if (q & WPAD_CLASSIC_BUTTON_HOME)   p |= PAD_TRIGGER_Z;
    }
  }
#endif

  return p;
}

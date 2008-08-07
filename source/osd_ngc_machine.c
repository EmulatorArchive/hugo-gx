/****************************************************************************
 * Hu-Go! Nintendo Gamecube
 *
 * machine functions
 ****************************************************************************/
#include "pce.h"
#include "utils.h"

char initial_path[PATH_MAX] = "";
// prefered path for for searching

UChar* osd_gfx_buffer = NULL;

UChar gamepad = 0;
// gamepad detected ?

UChar* XBuf;
// The screen buffer where we draw before blitting it on screen

int gamepad_driver = 0;
// what kind of jypad must we have to handle

char dump_snd = 0;
// Do we write sound to file

char synchro;
// … fond, … fond, … fond? (french joke ;)

int vwidth, vheight;
// size of visible part of the screen (I got troubles with allegro screen->* values!)

UInt32 interrupt_60hz(UInt32, void*);
// declaration of the actual callback to call 60 times a second

int osd_init_machine(void)
{

  Log ("\n--[ INITIALISE MACHINE ]--------------------------\n");
		
  if (!(XBuf = (UChar*)malloc(XBUF_WIDTH * XBUF_HEIGHT)))
    {
      printf (MESSAGE[language][failed_init]);
      return (0);
    }

  printf (MESSAGE[language][clear_buffer]);
  bzero (XBuf, XBUF_WIDTH * XBUF_HEIGHT);

  Log ("Initiating sound\n");
  printf (MESSAGE[language][init_sound]);
  
  InitSound();

  osd_gfx_buffer = XBuf + 32 + 64 * XBUF_WIDTH; // We skip the left border of 32 pixels and the 64 first top lines
	
  Log ("End of initialisation of the machine\n");

  return 1;
}


/*****************************************************************************

    Function: osd_shut_machine

    Description: Deinitialize all stuff that have been inited in osd_int_machine
    Parameters: none
    Return: nothing

*****************************************************************************/
void
osd_shut_machine (void)
{
 
}

/*****************************************************************************

    Function: osd_keypressed

    Description: Tells if a key is available for future call of osd_readkey
    Parameters: none
    Return: 0 is no key is available
            else any non zero value

*****************************************************************************/
SChar osd_keypressed(void)
{

#warning implement keypressed with sdl	
	
}

/*****************************************************************************

    Function: osd_readkey

    Description: Return the first available key stroke, waiting if needed
    Parameters: none
    Return: the key value (currently, lower byte is ascii and higher is scancode)

*****************************************************************************/
UInt16 osd_readkey(void)
{
}

 /*****************************************************************************

    Function: osd_fix_filename_slashes

    Description: Changes slashes in a filename to correspond to an os need
    Parameters: char* s
    Return: nothing but the char* is updated

*****************************************************************************/
void osd_fix_filename_slashes(char* s)
{
}

/*****************************************************************************

    Function: osd_init_paths

    Description: set global variables for paths and filenames
    Parameters: int argc, char* argv[]   same as the command line parameters
    Return: nothing

*****************************************************************************/
void
osd_init_paths(int argc, char* argv[])
{
}

/***************************************************************************
 *   SDCARD/MEMCARD File support
 *
 *
 ***************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat.h>
#include <sys/dir.h>

#include "saveicon.h"
#include "dvd.h"

#define SAVESIZE 0x4000

/* Support for MemCards  */
/**
 * libOGC System Work Area
 */
static u8 SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN (32);
static card_dir CardDir;
static card_file CardFile;
static card_stat CardStatus;

/**
 * DMA Transfer Area.
 * Must be 32-byte aligned.
 * 64k SRAM + 2k Icon
 */
static u8 savebuffer[SAVESIZE] ATTRIBUTE_ALIGN (32);

extern void WaitPrompt( char *prompt );
extern void ShowAction( char *text );
extern unsigned long ROMCRC32;
extern unsigned char *WRAM;
extern void ResetPCE();
extern void ResetSound();
extern unsigned int savetimer;



/****************************************************************************
 * SDCARD Access functions
 *
 * We use the same buffer as for Memory Card manager
 * Function returns TRUE on success.
 *****************************************************************************/
int SD_ManageFile(char *filename, int direction)
{
  char pathname[256];
  int done = 0;
  int filesize;
	
  /* first check if directory exist */
  DIR_ITER *dir = diropen("/hugo/saves");
  if (dir == NULL) mkdir("/hugo/saves",S_IRWXU);
  else dirclose(dir);

  /* build complete SDCARD filename */
  sprintf (pathname, "/hugo/saves/%s",filename);

  /* open file */
  FILE *fp = fopen(pathname, direction ? "rb" : "wb");
  if (fp == NULL)
	{
    sprintf (filename, "Error opening %s", pathname);
		WaitPrompt(filename);
		return 0;
	}
	
	switch (direction)
  {
		case 0: /* LOADING */
		
      /* read size */
      fseek(fp , 0 , SEEK_END);
      filesize = ftell (fp);
      fseek(fp, 0, SEEK_SET);

      /* read into buffer (32k blocks) */
      done = fread(savebuffer, 1, filesize, fp);
      if (done < filesize)
      {
        sprintf (filename, "Error reading %s", pathname);
		    WaitPrompt(filename);
		    return 0;
      }
      fclose(fp);

			memcpy (WRAM, &savebuffer[0], 0x2000);
			ResetPCE();
			ResetSound();
			savetimer = 0;
            
			sprintf (filename, "Loaded %d bytes successfully", done);
			WaitPrompt (filename);
			return 1;

		case 1: /* SAVING */

      memcpy (&savebuffer[0], WRAM, 0x2000);
			filesize = 0x2000;

      /* write buffer */
      done = fwrite(savebuffer, 1, filesize, fp);
      if (done < filesize)
      {
        sprintf (filename, "Error writing %s", pathname);
				WaitPrompt (filename);
				return 0;
			}

      fclose(fp);
      sprintf (filename, "Saved %d bytes successfully", done);
			WaitPrompt (filename);
			return 1;
		
	}
	
	return 0; 
}

/****************************************************************************
 * MountTheCard
 *
 * libOGC provides the CARD_Mount function, and it should be all you need.
 * However, experience with previous emulators has taught me that you are
 * better off doing a little bit more than that!
 *
 * Function returns TRUE on success.
 *****************************************************************************/
int MountTheCard (u8 slot)
{
	int tries = 0;
	int CardError;
  *(unsigned long *) (0xcc006800) |= 1 << 13; /*** Disable Encryption ***/
  uselessinquiry ();
  while (tries < 10)
  {
    VIDEO_WaitVSync ();
    CardError = CARD_Mount (slot, SysArea, NULL); /*** Don't need or want a callback ***/
    if (CardError == 0) return 1;
    else EXI_ProbeReset ();
    tries++;
	}
	return 0;
}

/****************************************************************************
 * CardFileExists
 *
 * Wrapper to search through the files on the card.
 * Returns TRUE if found.
 ****************************************************************************/
int CardFileExists (char *filename, u8 slot)
{
	int CardError = CARD_FindFirst (slot, &CardDir, TRUE);
	while (CardError != CARD_ERROR_NOFILE)
	{
		CardError = CARD_FindNext (&CardDir);
		if (strcmp ((char *) CardDir.filename, filename) == 0) return 1;
	}
	return 0;
}

/****************************************************************************
 * ManageWRAM
 *
 * Here is the main SRAM Management stuff.
 * The output file contains an icon (2K), 64 bytes comment and the SRAM (64k).
 * As memcards are allocated in blocks of 8k or more, you have a around
 * 6k bytes to save/load any other data you wish without altering any of the
 * main save / load code.
 *
 * direction == 0 save, 1 load.
 ****************************************************************************/
extern int hugoromsize;
extern unsigned char cart_reload;

int ManageWRAM (u8 direction, u8 device)
{
	char savefilename[128];
	int CardError;
	u32 SectorSize;
	char comment[2][32] = { "Hu-Go! 2.12 GC 0.0.2", "ANY GAME" };
  int outbytes = 0;
	int filesize;
	int offset;
	
	if (cart_reload || !hugoromsize) return 0;
 
  /* clean buffer */
  memset(savebuffer, 0, SAVESIZE);

	if (direction) ShowAction ("Saving WRAM ...");
	else ShowAction ("Loading WRAM ...");

	/*** Make savefilename ***/
	sprintf(savefilename, "%08lX.hgo", ROMCRC32);
	sprintf(comment[1], "CRC : %08lX", ROMCRC32);

	/* device is SDCARD, let's go */
	if (device == 0) return SD_ManageFile(savefilename,direction);

  u8 CARDSLOT = device - 1;

	/****** device is MCARD, we continue *****/
	/*** Initialise the memcard ***/
	memset(&SysArea, 0, CARD_WORKAREA);
	CARD_Init("HUGO","00");

	/*** Try to mount the card ***/
	CardError = MountTheCard(CARDSLOT);
	
	if (!CardError)
	{
		/*** Signal Failure ***/
		WaitPrompt("Unable to mount memory card");
		return 0;
	}
	CardError = CARD_GetSectorSize(CARDSLOT, &SectorSize);

	if (direction)	/*** Save ***/
	{
		/*** Setup savebuffer ***/
		memset(&savebuffer, 0, SAVESIZE);
		memcpy(&savebuffer, &saveicon, sizeof(saveicon));

		/*** Update the save buffer ***/
		memcpy(&savebuffer[sizeof(saveicon)], &comment[0], 64);
		outbytes = sizeof(saveicon) + 64;

		/*** Copy battery ram ***/
		memcpy(&savebuffer[outbytes], WRAM, 0x2000);
		outbytes += 0x2000;

		filesize = (outbytes / SectorSize) * SectorSize;
		if (outbytes % SectorSize) filesize += SectorSize;

		/*** Determine if this is a simple open, or new save ***/
		if (CardFileExists(savefilename, CARDSLOT))
		{
			/*** Open existing save handle ***/
			CardError = CARD_Open(CARDSLOT, savefilename, &CardFile);

			if (CardError)
			{
				WaitPrompt("Unable to open save");
				CARD_Unmount (CARDSLOT);
				return 0;
			}
		}
		else
		{
			/*** Create new save file ***/
			CardError = CARD_Create(CARDSLOT, savefilename, filesize, &CardFile);
			if (CardError)
			{
				WaitPrompt("Unable to create save");
				CARD_Unmount (CARDSLOT);
				return 0;
			}
		}

		/*** Now fill in the status ***/
		CARD_GetStatus(CARDSLOT, CardFile.filenum, &CardStatus);
		CardStatus.icon_addr = 0;
		CardStatus.icon_fmt = 2;
		CardStatus.icon_speed = 1;
		CardStatus.comment_addr = sizeof(saveicon);
		CARD_SetStatus(CARDSLOT, CardFile.filenum, &CardStatus);

		/*** And write the file out ***/
		offset = 0;
		while ( outbytes > 0 )
		{
			CardError = CARD_Write(&CardFile, &savebuffer[offset], SectorSize, offset);
			outbytes -= SectorSize;
			offset += SectorSize;
		}		

		CARD_Close(&CardFile);
		CARD_Unmount(CARDSLOT);
		sprintf(savefilename, "Saved %d bytes successfully", offset);
		WaitPrompt(savefilename);
	}
	else
	{	/*** Load ***/
		/*** Check for file ***/
		if (CardFileExists(savefilename, CARDSLOT))
		{
			memset(&CardFile, 0, sizeof(CardFile));
			CardError = CARD_Open( CARDSLOT, savefilename, &CardFile );
			if (CardError)
			{
				WaitPrompt("Unable to open save");
				CARD_Unmount (CARDSLOT);
				return 0;
			}

			sprintf(savefilename, "Filesize %d", CardFile.len);
			WaitPrompt(savefilename);

			if (CardFile.len != SAVESIZE)
			{
				WaitPrompt("Save is corrupt!");
				CARD_Unmount (CARDSLOT);
				return 0;
			}

			filesize = CardFile.len;
			offset = 0;
			while (filesize > 0)
			{
				CardError = CARD_Read( &CardFile, &savebuffer[offset], SectorSize, offset );
				filesize -= SectorSize;
				offset += SectorSize;
			}

			CARD_Close(&CardFile);
			CARD_Unmount(CARDSLOT);
	
			memcpy(WRAM, &savebuffer[sizeof(saveicon)+64], 0x2000);
			ResetPCE();
			ResetSound();
			savetimer = 0;
	
			WaitPrompt("Save successfully loaded");
		}
		else
		{   /*** No File ***/
			WaitPrompt("No save found!");
			return 0;
		}
	}

	return 1;
}

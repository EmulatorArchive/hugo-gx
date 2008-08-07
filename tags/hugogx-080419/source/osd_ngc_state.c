/****************************************************************************
 * Nintendo Gamecube Memcard Save/Load State
 *
 * Hu-Go! has a wonderful way of saving and loading. 
 * All relative information is held in the hard_pce structure.
 ****************************************************************************/
#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sdcard.h>
#include "saveicon.h"
#include "dvd.h"

/*** Memory card SysArea ***/
#define SAVESIZE 0x4000
static unsigned char SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN(32);
static unsigned char savebuffer[SAVESIZE] ATTRIBUTE_ALIGN(32);

card_dir CardDir;
card_file CardFile;
card_stat CardStatus;
int outbytes;
char debug[128];

extern void WaitPrompt( char *prompt );
extern void ShowAction( char *text );
extern unsigned long ROMCRC32;
extern unsigned char *WRAM;
extern void ResetPCE();
extern void ResetSound();
extern unsigned int savetimer;
extern int CARDSLOT;
extern int use_SDCARD;

/****************************************************************************
 * SDCARD Access functions
 *
 * We use the same buffer as for Memory Card manager
 * Function returns TRUE on success.
 *****************************************************************************/
int SD_ManageFile(char *filename, int direction)
{
    char name[1024];
	sd_file *handle;
    int len = 0;
    int offset = 0;
	int filesize;
	
	/* build complete SDCARD filename */
	sprintf (name, "dev%d:\\hugo\\saves\\%s", CARDSLOT, filename);

	/* open file */
	handle = direction ? SDCARD_OpenFile (name, "wb") :
						 SDCARD_OpenFile (name, "rb");

	if (handle == NULL)
	{
		sprintf (filename, "Error opening %s", name);
		WaitPrompt(filename);
		return 0;
	}
	
	switch (direction)
	{
		case 1: /* SAVING */

			memcpy (&savebuffer[0], WRAM, 0x2000);
			filesize = 0x2000;
			len = SDCARD_WriteFile (handle, savebuffer, filesize);
			SDCARD_CloseFile (handle);

			if (len != filesize)
			{
				sprintf (filename, "Error writing %s", name);
				WaitPrompt (filename);
				return 0;
			}
			
			sprintf (filename, "Saved %d bytes successfully", filesize);
			WaitPrompt (filename);
			return 1;
		
		case 0: /* LOADING */
		
			while ((len = SDCARD_ReadFile (handle, &savebuffer[offset], 2048)) > 0) offset += len;
			memcpy (WRAM, &savebuffer[0], 0x2000);
			ResetPCE();
			ResetSound();
			savetimer = 0;

            SDCARD_CloseFile (handle);

			sprintf (filename, "Loaded %d bytes successfully", offset);
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
int MountTheCard ()
{
	int tries = 0;
	int CardError;
	while (tries < 10)
	{
		*(unsigned long *) (0xcc006800) |= 1 << 13; /*** Disable Encryption ***/
		uselessinquiry ();
		VIDEO_WaitVSync ();
		CardError = CARD_Mount (CARDSLOT, SysArea, NULL); /*** Don't need or want a callback ***/
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
int CardFileExists (char *filename)
{
	int CardError = CARD_FindFirst (CARDSLOT, &CardDir, TRUE);
	while (CardError != CARD_ERROR_NOFILE)
	{
		CardError = CARD_FindNext (&CardDir);
		if (strcmp ((char *) CardDir.filename, filename) == 0) return 1;
	}
	return 0;
}

/****************************************************************************
 * Save / Load State
 ****************************************************************************/
extern int hugoromsize;
extern unsigned char cart_reload;

int doStateSaveLoad(int which)
{
	char savefilename[128];
	int CardError;
	int SectorSize;
	char comment[2][32] = { "Hu-Go! 2.12 GC 0.0.2", "ANY GAME" };
	int filesize;
	int offset;
	
	if (cart_reload || !hugoromsize) return 0;
	
	if (which) ShowAction ("Saving WRAM ...");
	else ShowAction ("Loading WRAM ...");

	/*** Make savefilename ***/
	sprintf(savefilename, "%08lX.hgo", ROMCRC32);
	sprintf(comment[1], "CRC : %08lX", ROMCRC32);

	/* device is SDCARD, let's go */
	if (use_SDCARD) return SD_ManageFile(savefilename,which);

	/****** device is MCARD, we continue *****/
	/*** Initialise the memcard ***/
	memset(&SysArea, 0, CARD_WORKAREA);
	CARD_Init("HUGO","00");

	/*** Try to mount the card ***/
	CardError = MountTheCard();
	
	if (!CardError)
	{
		/*** Signal Failure ***/
		WaitPrompt("Unable to mount memory card");
		return 0;
	}
	CardError = CARD_GetSectorSize(CARDSLOT, &SectorSize);

	if (which)	/*** Save ***/
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
		if (CardFileExists(savefilename))
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
		if (CardFileExists(savefilename))
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

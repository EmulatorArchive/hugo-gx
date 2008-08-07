/****************************************************************************
 * Nintendo Gamecube Memcard Save/Load State
 *
 * Hu-Go! has a wonderful way of saving and loading. 
 * All relative information is held in the hard_pce structure.
 ****************************************************************************/
#include "shared_memory.h"
#include <gccore.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"
#include "saveicon.h"
#include "osd_ngc_mix.h"
#include "cheat.h"

/*** Memory card SysArea ***/
#define SAVESIZE 0x4000
static unsigned char SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN(32);
static unsigned char savebuffer[SAVESIZE] ATTRIBUTE_ALIGN(32);
card_dir CardDir;
card_file CardFile;
card_stat CardStatus;
int CARDSLOT = CARD_SLOTA;
int outbytes;
extern unsigned long ROMCRC32;
extern void WaitPrompt( char *prompt );
char debug[128];
extern void uselessinquiry();

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
int doStateSaveLoad( int which , int card )
{

	char savefilename[128];
	int CardError;
	int SectorSize;
	char comment[2][32] = { "Hu-Go! 2.12 GC 0.0.2", "ANY GAME" };
	int filesize;
	int offset;
	
	/*** Initialise the memcard ***/
	CARDSLOT = card;
	memset(&SysArea, 0, CARD_WORKAREA);
	CARD_Init("HUGO","00");

	/*** Try to mount the card ***/
	CardError = MountTheCard();
	
	if ( CardError )
	{
		/*** Signal Failure ***/
		WaitPrompt("Unable to mount memory card");
		return 1;
	}

	/*** Make savefilename ***/
	sprintf(savefilename, "%08lX.hgo", ROMCRC32);
	sprintf(comment[1], "CRC : %08lX", ROMCRC32);

	CardError = CARD_GetSectorSize(CARDSLOT, &SectorSize);

	if ( which )	/*** Save ***/
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

		filesize = ( outbytes / SectorSize ) * SectorSize;
		if ( outbytes % SectorSize ) filesize += SectorSize;

		/*** Determine if this is a simple open, or new save ***/
		if ( CardFileExists( savefilename ) )
		{
			/*** Open existing save handle ***/
			CardError = CARD_Open(CARDSLOT, savefilename, &CardFile);

			if ( CardError )
			{
				WaitPrompt("Unable to open save");
				CARD_Unmount (CARDSLOT);
				return 1;
			}
		}
		else
		{
			/*** Create new save file ***/
			CardError = CARD_Create(CARDSLOT, savefilename, filesize, &CardFile);
			if ( CardError )
			{
				WaitPrompt("Unable to create save");
				CARD_Unmount (CARDSLOT);
				return 1;
			}
		}

		/*** Now fill in the status ***/
		CARD_GetStatus( CARDSLOT, CardFile.filenum, &CardStatus );
		CardStatus.icon_addr = 0;
		CardStatus.icon_fmt = 2;
		CardStatus.icon_speed = 1;
		CardStatus.comment_addr = sizeof(saveicon);
		CARD_SetStatus( CARDSLOT, CardFile.filenum, &CardStatus );

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
		if ( CardFileExists(savefilename) )
		{
			memset(&CardFile, 0, sizeof(CardFile));
			CardError = CARD_Open( CARDSLOT, savefilename, &CardFile );
			if ( CardError )
			{
				WaitPrompt("Unable to open save");
				CARD_Unmount (CARDSLOT);
				return 1;
			}

			sprintf(savefilename, "Filesize %d", CardFile.len);
			WaitPrompt(savefilename);

			if ( CardFile.len != SAVESIZE )
			{
				WaitPrompt("Save is corrupt!");
				CARD_Unmount (CARDSLOT);
				return 1;
			}

			filesize = CardFile.len;
			offset = 0;
			while ( filesize > 0 )
			{
				CardError = CARD_Read( &CardFile, &savebuffer[offset], SectorSize, offset );
				filesize -= SectorSize;
				offset += SectorSize;
			}

			CARD_Close( &CardFile );
			CARD_Unmount( CARDSLOT );
	
			memcpy(WRAM, &savebuffer[sizeof(saveicon)+64], 0x2000);
		
			WaitPrompt("Save successfully loaded");
	
		}
		else
		{   /*** No File ***/
			WaitPrompt("No save found!");
			return 0;
		}
	}

	return 0;

}


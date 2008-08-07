/****************************************************************************
 * DVD.CPP
 *
 * This module manages all dvd i/o etc.
 * There is also a simple ISO9660 parser included.
 ****************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sdcard.h>
#include "iplfont.h"
#include "gctime.h"

/** Minimal ISO Directory Definition **/
#define RECLEN 0			/* Record length */
#define EXTENT 6			/* Extent */
#define FILE_LENGTH 14		/* File length (BIG ENDIAN) */
#define FILE_FLAGS 25		/* File flags */
#define FILENAME_LENGTH 32	/* Filename length */
#define FILENAME 33			/* ASCIIZ filename */
#define MAXJOLIET 256

/* GUI Definition */
#define MAXFILES 1000
#define PAGESIZE 17
#define PADCAL 40

/* SDCARD reading & browsing */
int UseSDCARD = 0;
sd_file * filehandle;
char rootSDdir[SDCARD_MAX_PATH_LEN];
int haveSDdir = 0;

/* File selection */
typedef struct {
	unsigned int offset;
	unsigned int length;
	char flags;
    char filename[MAXJOLIET];
} FILEENTRIES;
int maxfiles = 0;
int offset = 0;
int selection = 0;
FILEENTRIES filelist[MAXFILES];

extern int IsZipFile(char *buffer);
extern int unzipDVDFile( unsigned char *outbuffer, unsigned int discoffset, unsigned int length);
extern void WaitPrompt( char *text );
extern int font_height;
extern void copybackdrop();
extern void ShowScreen();
extern void ShowAction( char *text );
extern int backcolour;
extern void ClearScreen();
extern unsigned char *hugorom;
extern int hugoromsize;
extern unsigned char cart_reload;
int LoadDVDFile( unsigned char *buffer );
void GetSDInfo () ;

/****************************************************************************
 * DVD Lowlevel Functions
 *
 * These are here simply because the same functions in libogc are not
 * exposed to the user
 ****************************************************************************/
#define DEBUG_STOP_DRIVE 0
#define DEBUG_START_DRIVE 0x100
#define DEBUG_ACCEPT_COPY 0x4000
#define DEBUG_DISC_CHECK 0x8000
extern int IsXenoGCImage( char *buffer );
extern void SendDriveCode( int model );
volatile unsigned long *dvd=(volatile unsigned long *)0xCC006000;

void dvd_inquiry()
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0x12000000;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = 0x80000000;
	dvd[6] = 0x20;
	dvd[7] = 3;

	while( dvd[7] & 1 );
	DCFlushRange((void *)0x80000000, 32);
}

/****************************************************************************
 * uselessinquiry
 *
 * As the name suggests, this function is quite useless.
 * It's only purpose is to stop any pending DVD interrupts while we use the
 * memcard interface.
 *
 * libOGC tends to foul up if you don't, and sometimes does if you do!
 ****************************************************************************/
void uselessinquiry ()
{

  dvd[0] = 0;
  dvd[1] = 0;
  dvd[2] = 0x12000000;
  dvd[3] = 0;
  dvd[4] = 0x20;
  dvd[5] = 0x80000000;
  dvd[6] = 0x20;
  dvd[7] = 1;

  while (dvd[7] & 1);
}

void dvd_unlock()
{
    dvd[0] |= 0x00000014;
    dvd[1] = 0x00000000;
    dvd[2] = 0xFF014D41;
    dvd[3] = 0x54534849;
    dvd[4] = 0x54410200;
    dvd[7] = 1;
    while ((dvd[0] & 0x14) == 0) { }
    dvd[0] |= 0x00000014;
    dvd[1] = 0x00000000;
    dvd[2] = 0xFF004456;
    dvd[3] = 0x442D4741;
    dvd[4] = 0x4D450300;
    dvd[7] = 1;
    while ((dvd[0] & 0x14) == 0) { }
}

void dvd_extension()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0x55010000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // enable reading!
	while (dvd[7] & 1);
}


void dvd_motor_on_extra()
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0xfe110000 | DEBUG_START_DRIVE | DEBUG_ACCEPT_COPY | DEBUG_DISC_CHECK;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;
	while ( dvd[7] & 1 );
}

void dvd_motor_off( )
{
	dvd[0] = 0x2e;
	dvd[1] = 0;
	dvd[2] = 0xe3000000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // Do immediate
	while (dvd[7] & 1);

	/*** PSO Stops blackscreen at reload ***/
	dvd[0] = 0x14;
	dvd[1] = 0;
}

void dvd_setstatus()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xee060300;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // enable reading!
	while (dvd[7] & 1);
}

unsigned int dvd_read_id(void *dst)
{
	if ((((int)dst) & 0xC0000000) == 0x80000000) // cached?
		DCInvalidateRange((void *)dst, 0x20);

	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = (unsigned long)dst;
	dvd[6] = 0x20;
	dvd[7] = 3; // enable reading!

	while (dvd[7] & 1);

	if (dvd[0] & 0x4) return 1;
	
	return 0;
}

void dvd_reset(void)
{
   *(unsigned long*)0xcc006004 = 2;
   unsigned long v = *(unsigned long*)0xcc003024;
   *(unsigned long*)0xcc003024 = (v &~4) | 1;
   mdelay(2);
   *(unsigned long*)0xcc003024 = v | 5;
}

void dvd_reset_ipl( void )
{
        dvd[1] = 2;
        *(unsigned int *)0xCC003030 = 0x245248a;
        unsigned int p = *(unsigned int *)0xcc003024;
        *(unsigned int *)0xcc003024 = ( p & ~4 ) | 1;
        mdelay(1);
        *(unsigned int *)0xcc003024 = p | 5;
}

/** Due to lack of memory, we'll use this little 2k keyhole for all DVD operations **/
unsigned char DVDreadbuffer[2048] ATTRIBUTE_ALIGN (32);

int dvd_read (void *dst, unsigned int len, unsigned int offset)
{
  unsigned char *buffer = (unsigned char *) (unsigned int) DVDreadbuffer;
  if (len > 2048) return 1; /*** We only allow 2k reads **/

  DCInvalidateRange( (void *)buffer, len );
  if (offset < 0x57057C00)	/*** Must be < 1.35GB ***/
  {
      dvd[0] = 0x2E;
      dvd[1] = 0;
      dvd[2] = 0xA8000000;
      dvd[3] = offset >> 2;
      dvd[4] = len;
      dvd[5] = (unsigned long) buffer;
      dvd[6] = len;
      dvd[7] = 3; /*** Enable reading with DMA ***/
      while (dvd[7] & 1);
      memcpy (dst, buffer, len);
  }
  else return 0; // Let's not read past end of DVD
   
  if (dvd[0] & 0x4) return 0; /* Ensure it has completed */
  
  return 1;
}

/****************************************************************************
 * ISO Parsing Functions
 ****************************************************************************/
#define PVDROOT 0x9c
static int IsJoliet = 0;
static int rootdir = 0;
static int rootdirlength = 0;
static char dvdbuffer[2048];

/****************************************************************************
 * Primary Volume Descriptor
 *
 * The PVD should reside between sector 16 and 31.
 * This is for single session DVD only.
 ****************************************************************************/
int getpvd ()
{
	int sector = 16;
	rootdir = rootdirlength = 0;
	IsJoliet = -1;

	/** Look for Joliet PVD first **/
	while (sector < 32)
    {
		if (dvd_read (&dvdbuffer, 2048, sector << 11))
		{
			if (memcmp (&dvdbuffer, "\2CD001\1", 8) == 0)
			{
				memcpy (&rootdir, &dvdbuffer[PVDROOT + EXTENT], 4);
				memcpy (&rootdirlength, &dvdbuffer[PVDROOT + FILE_LENGTH], 4);
				rootdir <<= 11;
				IsJoliet = 1;
				break;
			}
		}
		else return 0; /*** Can't read sector! ***/

		sector++;
    }

	if (IsJoliet > 0) return 1; /*** Joliet PVD Found ? ***/
    
	sector = 16;

	/*** Look for standard ISO9660 PVD ***/
	while (sector < 32)
    {
		if (dvd_read (&dvdbuffer, 2048, sector << 11))
		{
			if (memcmp (&dvdbuffer, "\1CD001\1", 8) == 0)
			{
				memcpy (&rootdir, &dvdbuffer[PVDROOT + EXTENT], 4);
				memcpy (&rootdirlength, &dvdbuffer[PVDROOT + FILE_LENGTH], 4);
				IsJoliet = 0;
				rootdir <<= 11;
				break;
			}
		}
		else return 0; /*** Can't read sector! ***/
	
		sector++;
    }

	return (IsJoliet == 0);
}


/****************************************************************************
 * getentry
 *
 * Support function to return the next file entry, if any
 * Declared static to avoid accidental external entry.
 ****************************************************************************/
static int diroffset = 0;
static int getentry (int entrycount)
{
	char fname[512]; /* Huge, but experience has determined this */
	char *ptr;
	char *filename;
	char *filenamelength;
	char *rr;
	int j;

	/* Basic checks */
	if (entrycount >= MAXFILES) return 0;
	if (diroffset >= 2048) return 0;

	/** Decode this entry **/
	if (dvdbuffer[diroffset])	/* Record length available */
    {
		/* Update offsets into sector buffer */
		ptr = (char *) &dvdbuffer[0];
		ptr += diroffset;
		filename = ptr + FILENAME;
		filenamelength = ptr + FILENAME_LENGTH;

		/* Check for wrap round - illegal in ISO spec,
		 * but certain crap writers do it! */
		if ((diroffset + dvdbuffer[diroffset]) > 2048) return 0;

		if (*filenamelength)
		{
			memset (&fname, 0, 512);

			/*** Do ISO 9660 first ***/
			if (!IsJoliet) strcpy (fname, filename);
			else
			{
				/*** The more tortuous unicode joliet entries ***/
				for (j = 0; j < (*filenamelength >> 1); j++)
				{
					fname[j] = filename[j * 2 + 1];
				}

				fname[j] = 0;

				if (strlen (fname) >= MAXJOLIET) fname[MAXJOLIET - 1] = 0;
				if (strlen (fname) == 0) fname[0] = filename[0];
			}

			if (strlen (fname) == 0) strcpy (fname, "ROOT");
			else
			{
				if (fname[0] == 1) strcpy (fname, "..");
				else
				{
					/*
					 * Move *filenamelength to t,
					 * Only to stop gcc warning for noobs :)
					 */
					int t = *filenamelength;
					fname[t] = 0;
				}
			}

			/** Rockridge Check **/
			rr = strstr (fname, ";");
			if (rr != NULL) *rr = 0;

			strcpy (filelist[entrycount].filename, fname);
			memcpy (&filelist[entrycount].offset, &dvdbuffer[diroffset + EXTENT], 4);
			memcpy (&filelist[entrycount].length, &dvdbuffer[diroffset + FILE_LENGTH], 4);
			memcpy (&filelist[entrycount].flags, &dvdbuffer[diroffset + FILE_FLAGS], 1);

			filelist[entrycount].offset <<= 11;
			filelist[entrycount].flags = filelist[entrycount].flags & 2;

			/*** Prepare for next entry ***/
			diroffset += dvdbuffer[diroffset];

			return 1;
		}
	}
	return 0;
}

/****************************************************************************
 * parsedirectory
 *
 * This function will parse the directory tree.
 * It relies on rootdir and rootdirlength being pre-populated by a call to
 * getpvd, a previous parse or a menu selection.
 *
 * The return value is number of files collected, or 0 on failure.
 ****************************************************************************/
int parsedirectory ()
{
	int pdlength;
	int pdoffset;
	int rdoffset;
	int len = 0;
	int filecount = 0;

	pdoffset = rdoffset = rootdir;
	pdlength = rootdirlength;
	filecount = 0;

	/** Clear any existing values ***/
	memset (&filelist, 0, sizeof (FILEENTRIES) * MAXFILES);

	/*** Get as many files as possible ***/
	while (len < pdlength)
	{
		if (dvd_read (&dvdbuffer, 2048, pdoffset) == 0) return 0;

		diroffset = 0;

		while (getentry (filecount))
		{
			if (filecount < MAXFILES) filecount++;
		}

		len += 2048;
		pdoffset = rdoffset + len;
	}
	return filecount;
}

/***************************************************************************
 * Update SDCARD curent directory name 
 ***************************************************************************/ 
int updateSDdirname()
{
  int size=0;
  char *test;
  char temp[1024];

   /* current directory doesn't change */
   if (strcmp(filelist[selection].filename,".") == 0) return 0; 
   
   /* go up to parent directory */
   else if (strcmp(filelist[selection].filename,"..") == 0) 
   {
     /* determine last subdirectory namelength */
     sprintf(temp,"%s",rootSDdir);
     test= strtok(temp,"\\");
     while (test != NULL)
     { 
       size = strlen(test);
       test = strtok(NULL,"\\");
     }
  
     /* remove last subdirectory name */
     size = strlen(rootSDdir) - size - 1;
     rootSDdir[size] = 0;

	 /* handles root name */
	 if (strcmp(rootSDdir,"dev0:") == 0) sprintf(rootSDdir,"dev0:\\HUGOROMS\\..");
	 
     return 1;
   }
   else
   {
     /* test new directory namelength */
     if ((strlen(rootSDdir)+1+strlen(filelist[selection].filename)) < SDCARD_MAX_PATH_LEN) 
     {
       /* handles root name */
	   if (strcmp(rootSDdir,"dev0:\\HUGOROMS\\..") == 0) sprintf(rootSDdir,"dev0:");
	 
       /* update current directory name */
       sprintf(rootSDdir, "%s\\%s",rootSDdir, filelist[selection].filename);
       return 1;
     }
     else
     {
         WaitPrompt ("Dirname is too long !"); 
         return -1;
     }
    } 
}

/***************************************************************************
 * Browse SDCARD subdirectories 
 ***************************************************************************/ 
int parseSDdirectory()
{
  int entries = 0;
  int nbfiles = 0;
  DIR *sddir = NULL;

  /* initialize selection */
  selection = offset = 0;
  
  /* Get a list of files from the actual root directory */ 
  entries = SDCARD_ReadDir (rootSDdir, &sddir);
  
  if (entries < 0) entries = 0;   
  if (entries > MAXFILES) entries = MAXFILES;
    
  /* Move to DVD structure - this is required for the file selector */ 
  while (entries)
  {
      memset (&filelist[nbfiles], 0, sizeof (FILEENTRIES));
      strncpy(filelist[nbfiles].filename,sddir[nbfiles].fname,MAXJOLIET);
      filelist[nbfiles].filename[MAXJOLIET-1] = 0;
	  filelist[nbfiles].length = sddir[nbfiles].fsize;
	  filelist[nbfiles].flags = (char)(sddir[nbfiles].fattr & SDCARD_ATTR_DIR);
      nbfiles++;
      entries--;
  }
  
  /*** Release memory ***/
  free(sddir);
  
  return nbfiles;
}

/****************************************************************************
 * ShowFiles
 *
 * Support function for FileSelector
 ****************************************************************************/

void ShowFiles( int offset, int selection )
{
	int i,j;
	char text[80];

	ClearScreen();

	j = 0;
	for ( i = offset; i < ( offset + PAGESIZE ) && ( i < maxfiles ); i++ )
	{
		if ( filelist[i].flags ) {
			strcpy(text,"[");
			strcat(text, filelist[i].filename);
			strcat(text,"]");
		} else
			strcpy(text, filelist[i].filename);
		
		if ( j == ( selection - offset ) )
			write_centre_hi( ( j * font_height ) + 32, text );
		else
			write_centre( ( j * font_height ) + 32, text );
		j++;		
	}

	ShowScreen();
}

/****************************************************************************
 * FileSelector
 *
 * Let user select another ROM to load
 ****************************************************************************/

void FileSelector()
{
	short p;
	signed char a;
	int haverom = 0;
	int redraw = 1;

	while (haverom == 0)
	{
		if (redraw) ShowFiles(offset, selection);
		redraw = 0;

		p = PAD_ButtonsDown(0);
		a = PAD_StickY(0);

        if (p & PAD_BUTTON_B) haverom = 1;
        
		if ((p & PAD_BUTTON_DOWN) || (a < -PADCAL))
        {
           selection++;
           if ( selection == maxfiles ) selection = offset = 0;
           if ( ( selection - offset ) >= PAGESIZE )  offset += PAGESIZE;
           redraw = 1;
        } // End of down

        if ((p & PAD_BUTTON_UP) || (a > PADCAL))
        {
          selection--;
          if (selection < 0)
		  {
			  selection = maxfiles - 1;
              offset = selection - PAGESIZE + 1;
          }

          if (selection < offset) offset -= PAGESIZE;
          if (offset < 0) offset = 0;
          redraw = 1;
        } // End of Up

        if (p & PAD_BUTTON_LEFT)
        {
          /*** Go back a page ***/
          selection -= PAGESIZE;
          if ( selection < 0 )
		  {
            selection = maxfiles - 1;
            offset = selection-PAGESIZE + 1;
          }

          if (selection < offset ) offset -= PAGESIZE;
          if ( offset < 0 ) offset = 0;
		  redraw = 1;
        }

        if (p & PAD_BUTTON_RIGHT)
        {
          /*** Go forward a page ***/
          selection += PAGESIZE;
          if (selection > maxfiles - 1) selection = offset = 0;
          if ( ( selection - offset ) >= PAGESIZE ) offset += PAGESIZE;
	      redraw = 1;
        }

		if (p & PAD_BUTTON_A)
		{
		  if ( filelist[selection].flags )	/*** This is directory ***/
		  {
			  if (UseSDCARD)
              {
				  /* update current directory and set new entry list if directory has changed */
		          int status = updateSDdirname();
		          if (status == 1)
		          {
					  maxfiles = parseSDdirectory();
		              if (!maxfiles)
		              {
						  WaitPrompt ("Error reading directory !");
		                  haverom   = 1; // quit SD menu
                          haveSDdir = 0; // reset everything at next access
			          }
		          }
		          else if (status == -1)
                  {
					  haverom   = 1; // quit SD menu
                      haveSDdir = 0; // reset everything at next access
                  }
               }
			   else
			   {
				   rootdir = filelist[selection].offset;
				   rootdirlength = filelist[selection].length;
				   offset = selection = 0;
				   maxfiles = parsedirectory();
			   }
		  }
		  else
		  {
			  rootdir = filelist[selection].offset;
			  rootdirlength = filelist[selection].length;
			  /*** Put ROM Load Routine Here :) ***/
              hugoromsize = LoadDVDFile(&hugorom[0]);
			  cart_reload = 1;
			  haverom = 1;
	      }
		  redraw = 1;
        }
	}
}


/****************************************************************************
 * LoadDVDFile
****************************************************************************/
int LoadDVDFile( unsigned char *buffer )
{
  int offset;
  int blocks;
  int i;
  int discoffset;
  char readbuffer[2048];
  
  /*** SDCard Addition ***/ 
  if (UseSDCARD) GetSDInfo();

  if (rootdirlength == 0) return 0;

  /*** How many 2k blocks to read ***/
  blocks = rootdirlength / 2048;
  offset = 0;
  discoffset = rootdir;

  ShowAction("Loading ... Wait");
  if (UseSDCARD) SDCARD_ReadFile (filehandle, &readbuffer, 2048);
  else dvd_read(&readbuffer, 2048, discoffset);

  if (!IsZipFile ((char *) &readbuffer))
  {
     if (UseSDCARD) SDCARD_SeekFile (filehandle, 0, SDCARD_SEEK_SET);
     
     /*** 0.0.2 - Addition of GCD images ***/
	 if ( memcmp(&readbuffer, "TG16GCCD", 8) == 0 )
	 {
        /*** Do TG16 Load ***/
	    WaitPrompt("TG16GCCD");
     }

     for ( i = 0; i < blocks; i++ )
	 {
	    if (UseSDCARD) SDCARD_ReadFile (filehandle, &readbuffer, 2048);
	    else dvd_read(&readbuffer, 2048, discoffset);
		memcpy(&buffer[offset], &readbuffer, 2048);
		offset += 2048;
		discoffset += 2048;
	 }
	
	 /*** And final cleanup ***/
	 if( rootdirlength % 2048 )
	 {
	    i = rootdirlength % 2048;
	    if (UseSDCARD) SDCARD_ReadFile (filehandle, &readbuffer, i);
	    else dvd_read(&readbuffer, 2048, discoffset);
		memcpy(&buffer[offset], &readbuffer, i);
	 }
  }
  else
  {
     return unzipDVDFile( buffer, discoffset, rootdirlength);
  }

  if (UseSDCARD) SDCARD_CloseFile (filehandle);
  return rootdirlength;
}


/****************************************************************************
 * OpenDVD
 *
 ****************************************************************************/
static int havedir = 0;

int OpenDVD()
{
	UseSDCARD = 0;
		
	while (!getpvd())
	{
		ShowAction("Mounting DVD ... Wait");
	    DVD_Mount();
	}

	if (havedir == 0)
	{	
		/* don't mess with SD entries */
		haveSDdir = 0;

		offset = selection = 0;

		if ((maxfiles = parsedirectory()))
		{
			FileSelector();
			havedir = 1;
		} 
	}
	else FileSelector();

	return 1;		
}


/****************************************************************************
 * OpenSD updated to use the new libogc.  Written by softdev and pasted
 * into this code by Drack.
 * Modified for subdirectory browing & quick filelist recovery
 * Enjoy!
*****************************************************************************/
int OpenSD () 
{
  UseSDCARD = 1;
  
  if (haveSDdir == 0)
  {
     /* don't mess with DVD entries */
	 havedir = 0;
	 
	 /* Initialise libOGC SD functions */ 
     SDCARD_Init ();
 
     /* Reset SDCARD root directory */
     sprintf(rootSDdir,"dev0:\\HUGOROMS");
 
	 /* Parse initial root directory and get entries list */
     if ((maxfiles = parseSDdirectory ()))
	 {
       /* Select an entry */
	   FileSelector ();
    
       /* memorize last entries list, actual root directory and selection for next access */
	     haveSDdir = 1;
	 }
	 else
     {
        /* no entries found */
	    WaitPrompt ("HUGOROMS not found on SDCARD (slot A) !");
        return 0;
	 }
  }
  /* Retrieve previous entries list and made a new selection */
  else  FileSelector ();
  
  return 1;
}

/****************************************************************************
 * SDCard Get Info
 ****************************************************************************/ 
void GetSDInfo () 
{
  char fname[SDCARD_MAX_PATH_LEN];
  rootdirlength = 0;
 
  /* Check filename length */
  if ((strlen(rootSDdir)+1+strlen(filelist[selection].filename)) < SDCARD_MAX_PATH_LEN)
     sprintf(fname, "%s\\%s",rootSDdir,filelist[selection].filename); 
  
  else
  {
    WaitPrompt ("Maximum Filename Length reached !"); 
    haveSDdir = 0; // reset everything before next access
  }

  filehandle = SDCARD_OpenFile (fname, "rb");
  if (filehandle == NULL)
    {
      WaitPrompt ("Unable to open file!");
      return;
    }
  rootdirlength = SDCARD_GetFileSize (filehandle);
}

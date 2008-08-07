/****************************************************************************
 * ROM Selection Interface
 *
 * The following features are implemented:
 *   . SDCARD access with LFN support (through softdev's VFAT library)
 *   . DVD access
 *   . easy subdirectory browsing
 *   . ROM browser
 *
 ***************************************************************************/
#include <pce.h>
#include <gccore.h>
#include <ogcsys.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat.h>
#include <sys/dir.h>

#include "dvd.h"
#include "iso9660.h"
#include "font.h"
#include "unzip.h"

#define PAGESIZE 12
#define PADCAL 60


static int maxfiles;
static int offset = 0;
static int selection = 0;
static int old_selection = 0;
static int old_offset = 0;
static char rootSDdir[256];
static u8 haveDVDdir = 0;
static u8 haveSDdir  = 0;
static u8 UseSDCARD = 0;
static int LoadFile (unsigned char *buffer);


extern void ShowScreen();
extern void ClearScreen();
extern void WaitPrompt(char *prompt);
extern void ShowAction(char *prompt);
extern int hugoromsize;
extern unsigned char *hugorom;

/* globals */
FILE *sdfile;


/***************************************************************************
 * ShowFiles
 *
 * Show filenames list in current directory
 ***************************************************************************/
static void ShowFiles (int offset, int selection) 
{
	int i,j;
  char text[MAXJOLIET+2];

	ClearScreen();
	j = 0;

	for (i=offset; i<(offset + PAGESIZE) && (i<maxfiles); i++)
	{
	  memset(text,0,MAXJOLIET+2);
	  if (filelist[i].flags) sprintf(text, "[%s]", filelist[i].filename + filelist[i].filename_offset);
	  else sprintf (text, "%s", filelist[i].filename + filelist[i].filename_offset);
	  if (j == (selection - offset)) write_centre_hi ((j * font_height) + 32, text);
    else write_centre ((j * font_height) + 32, text);
	  j++;		
	}
	ShowScreen();
}

/***************************************************************************
 * updateSDdirname
 *
 * Update ROOT directory while browsing SDCARD
 ***************************************************************************/ 
static int updateSDdirname()
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
    test= strtok(temp,"/");
    while (test != NULL)
    {
      size = strlen(test);
      test = strtok(NULL,"/");
    }

    /* remove last subdirectory name */
    size = strlen(rootSDdir) - size;
    rootSDdir[size-1] = 0;
  }
  else
  {
    sprintf(rootSDdir, "%s%s/",rootSDdir, filelist[selection].filename);
  }

  return 1;
}

/***************************************************************************
 * FileSortCallback (submitted by Marty Disibio)
 *
 * Quick sort callback to sort file entries with the following order:
 *   .
 *   ..
 *   <dirs>
 *   <files>
 ***************************************************************************/ 
static int FileSortCallback(const void *f1, const void *f2)
{
	/* Special case for implicit directories */
	if(((FILEENTRIES *)f1)->filename[0] == '.' || ((FILEENTRIES *)f2)->filename[0] == '.')
	{
		if(strcmp(((FILEENTRIES *)f1)->filename, ".") == 0) { return -1; }
		if(strcmp(((FILEENTRIES *)f2)->filename, ".") == 0) { return 1; }
		if(strcmp(((FILEENTRIES *)f1)->filename, "..") == 0) { return -1; }
		if(strcmp(((FILEENTRIES *)f2)->filename, "..") == 0) { return 1; }
	}
	
	/* If one is a file and one is a directory the directory is first. */
	if(((FILEENTRIES *)f1)->flags == 1 && ((FILEENTRIES *)f2)->flags == 0) return -1;
	if(((FILEENTRIES *)f1)->flags == 0 && ((FILEENTRIES *)f2)->flags == 1) return 1;
	
	return stricmp(((FILEENTRIES *)f1)->filename, ((FILEENTRIES *)f2)->filename);
}

/***************************************************************************
 * parseSDdirectory
 *
 * List files into one SDCARD directory
 ***************************************************************************/ 
static int parseSDdirectory()
{
  int nbfiles = 0;
  char filename[MAXPATHLEN];
  struct stat filestat;

  /* open directory */
  DIR_ITER *dir = diropen (rootSDdir);
  if (dir == NULL) 
  {
    sprintf(filename, "Error opening %s", rootSDdir);
    WaitPrompt (filename);
    return 0;
  }

  while (dirnext(dir, filename, &filestat) == 0)
  {
    if (strcmp(filename,".") != 0)
    {
      memset(&filelist[nbfiles], 0, sizeof (FILEENTRIES));
      sprintf(filelist[nbfiles].filename,"%s",filename);
      filelist[nbfiles].length = filestat.st_size;
      filelist[nbfiles].flags = (filestat.st_mode & S_IFDIR) ? 1 : 0;
      nbfiles++;
    }
  }
 
  dirclose(dir);

  /* Sort the file list */
  qsort(filelist, nbfiles, sizeof(FILEENTRIES), FileSortCallback);
  
  return nbfiles;
}

/****************************************************************************
 * FileSelector
 *
 * Let user select a file from the File listing
 ****************************************************************************/
extern void reloadrom ();
extern short ogc_input__getMenuButtons(void);

static void FileSelector()
{
	short p;
	int haverom = 0;
	int redraw = 1;
  int go_up = 0;
	int i,size;

	while (haverom == 0)
	{
		if (redraw) ShowFiles(offset, selection);
		redraw = 0;
		p = ogc_input__getMenuButtons();

		/* scroll displayed filename */
  	if (p & PAD_BUTTON_LEFT)
		{
			if (filelist[selection].filename_offset > 0)
			{
				filelist[selection].filename_offset --;
				redraw = 1;
			}
		}
		else if (p & PAD_BUTTON_RIGHT)
		{
			size = 0;
			for (i=filelist[selection].filename_offset; i<strlen(filelist[selection].filename); i++)
				size += font_size[(int)filelist[selection].filename[i]];
		  
			if (size > back_framewidth)
			{
				filelist[selection].filename_offset ++;
				redraw = 1;
			}
		}

		/* highlight next item */
		else if (p & PAD_BUTTON_DOWN)
		{
			filelist[selection].filename_offset = 0;
			selection++;
			if (selection == maxfiles) selection = offset = 0;
			if ((selection - offset) >= PAGESIZE) offset += PAGESIZE;
			redraw = 1;
		}

		/* highlight previous item */
		else if (p & PAD_BUTTON_UP)
		{
			filelist[selection].filename_offset = 0;
			selection--;
			if (selection < 0)
			{
				selection = maxfiles - 1;
				offset = selection - PAGESIZE + 1;
			}
			if (selection < offset) offset -= PAGESIZE;
			if (offset < 0) offset = 0;
			redraw = 1;
		}
      
		/* go back one page */
		else if (p & PAD_TRIGGER_L)
		{
			filelist[selection].filename_offset = 0;
			selection -= PAGESIZE;
			if (selection < 0)
			{
				selection = maxfiles - 1;
				offset = selection - PAGESIZE + 1;
			}
			if (selection < offset) offset -= PAGESIZE;
			if (offset < 0) offset = 0;
			redraw = 1;
		}

		/* go forward one page */
		else if (p & PAD_TRIGGER_R)
		{
			filelist[selection].filename_offset = 0;
			selection += PAGESIZE;
			if (selection > maxfiles - 1) selection = offset = 0;
			if ((selection - offset) >= PAGESIZE) offset += PAGESIZE;
			redraw = 1;
		}
		
		/* go up one directory or quit */
		if (p & PAD_BUTTON_B)
		{
			filelist[selection].filename_offset = 0;
      if (UseSDCARD)
      {
        if (strcmp(filelist[0].filename,"..") != 0) return;
      }
      else
      {
        if (basedir == rootdir) return;
      }
      go_up = 1;
    }
   
		/* quit */
		if (p & PAD_TRIGGER_Z)
		{
			filelist[selection].filename_offset = 0;
			return;
		}

		/* open selected file or directory */
		if ((p & PAD_BUTTON_A) || go_up)
		{
			filelist[selection].filename_offset = 0;
			if (go_up)
			{
        /* select item #1 */
				go_up = 0;
        selection = UseSDCARD ? 0 : 1;
			}

			/*** This is directory ***/
			if (filelist[selection].flags)
			{
				/* SDCARD directory handler */
        if (UseSDCARD)
				{
					/* update current directory */
					if (updateSDdirname())
					{
						/* reinit selector (previous value is saved for one level) */
						if (selection == 0)
						{
							selection = old_selection;
							offset = old_offset;
							old_selection = 0;
							old_offset = 0;
						}
						else
						{
							/* save current selector value */
							old_selection = selection;
							old_offset = offset;
							selection = 0;
							offset = 0;
						}
						
						/* set new entry list */
						maxfiles = parseSDdirectory();
						if (!maxfiles)
						{
							/* quit */
							WaitPrompt ("No files found !");
							haverom   = 1;
							haveSDdir = 0;
						}
					}
				}
				else /* DVD directory handler */
				{
					/* move to a new directory */
					if (selection != 0)
					{
						/* update current directory */
						rootdir = filelist[selection].offset;
						rootdirlength = filelist[selection].length;
				  
						/* reinit selector (previous value is saved for one level) */
						if (selection == 1)
						{
							selection = old_selection;
							offset = old_offset;
							old_selection = 0;
							old_offset = 0;
						}
						else
						{
							/* save current selector value */
							old_selection = selection;
							old_offset = offset;
							selection = 0;
							offset = 0;
						}

						/* get new entry list */
						maxfiles = parseDVDdirectory ();
					}
				}
			}
			else /*** This is a file ***/
			{
				rootdir = filelist[selection].offset;
				rootdirlength = filelist[selection].length;
        hugoromsize = LoadFile(hugorom);
			  cart_reload = 1;
				haverom = 1;
			}
			redraw = 1;
		}
	}
}

/****************************************************************************
 * OpenDVD
 *
 * Function to load a DVD directory and display to user.
 ****************************************************************************/
void OpenDVD() 
{
  UseSDCARD = 0;
  if (!getpvd())
  {
	  ShowAction("Mounting DVD ... Wait");
	  DVD_Mount();
	  haveDVDdir = 0;
	  if (!getpvd())
	  {
		  WaitPrompt ("Failed to mount DVD");
      return;
	  }
  }
  
  if (haveDVDdir == 0)
  {
    /* don't mess with SD entries */
	  haveSDdir = 0;
	 
	  /* reinit selector */
    rootdir = basedir;
	  old_selection = selection = offset = old_offset = 0;
    
    if ((maxfiles = parseDVDdirectory ()))
	  {
	    FileSelector ();
      haveDVDdir = 1;
	  }
  }
  else FileSelector ();
}
   
/****************************************************************************
 * OpenSD
 *
 * Function to load a SDCARD directory and display to user.
 ****************************************************************************/ 
int OpenSD()
{
  UseSDCARD = 1;
  if (haveSDdir == 0)
  {
    /* don't mess with DVD entries */
    haveDVDdir = 0;
	 
    /* reinit selector */
	  old_selection = selection = offset = old_offset = 0;
	
    /* Reset SDCARD root directory */
    sprintf (rootSDdir, "/hugo/roms/");

    /* if directory doesn't exist, use root */
    DIR_ITER *dir = diropen(rootSDdir);
    if (dir == NULL) sprintf (rootSDdir, "fat:/");
    else dirclose(dir);
  }
 
  /* Parse root directory and get entries list */
  ShowAction("Reading Directory ...");
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
    WaitPrompt ("no files found !");
    haveSDdir = 0;
		return 0;
	}
  
  return 1;
}


/****************************************************************************
 * LoadFile
 *
 * This function will load a file from DVD or SDCARD, in BIN, SMD or ZIP format.
 * The values for offset and length are inherited from rootdir and 
 * rootdirlength.
 *
 * The buffer parameter should re-use the initial ROM buffer.
 ****************************************************************************/ 
static int LoadFile (unsigned char *buffer) 
{
  int readoffset;
  int blocks;
  int i;
  u64 discoffset = 0;
  char readbuffer[2048];
  char fname[MAXPATHLEN];

  CD_emulation = 0;
  
  /* SDCard access */ 
  if (UseSDCARD)
  {
    /* complete filename */
    sprintf(fname, "%s%s",rootSDdir,filelist[selection].filename);

    /* CDROM image support */
    if (strcasestr (fname, ".HCD"))
    {
      /* Hu-Go! Cd Definition */
      CD_emulation = 5;
      if (!fill_HCD_info (fname)) return 0;
    }
    else if (strcasestr (fname, ".ISO"))
    {
      /* ISO support */
      CD_emulation = 2;
    }
    else if (strcasestr (fname, ".BIN"))
    {
      /*  BIN support */
      CD_emulation = 4;
    }

    if (CD_emulation)
    {
      /* Load SYSCARD rom */
      sdfile = fopen("/hugo/syscard.pce", "rb");
      if (sdfile == NULL)
      {
        CD_emulation = 0;
        WaitPrompt ("/hugo/syscard.pce not found !");
        return 0;
      }

      /* find SYSCARD rom size */
      fseek (sdfile, 0, SEEK_END);
      rootdirlength = ftell(sdfile);

      /* ajust size if header present */
      fseek (sdfile, rootdirlength & 0x1fff, SEEK_SET);
      rootdirlength &= ~0x1fff;

      /* read SYSCARD */
      fread (buffer, 1, rootdirlength, sdfile);
      fclose (sdfile);

      /* Load correct ISO filename */
      strcpy (ISO_filename, fname);

      return rootdirlength;
    }

    /* Normal ROM */
    sdfile = fopen(fname, "rb");
    if (sdfile == NULL)
    {
      WaitPrompt ("Unable to open file!");
      haveSDdir = 0;
      return 0;
    }
  }

  /* How many 2k blocks to read */ 
  if (rootdirlength == 0) return 0;
  blocks = rootdirlength / 2048;
  readoffset = 0;

  ShowAction("Loading ... Wait");

  /* Read first data chunk */
  if (UseSDCARD)
  {
    fread(readbuffer, 1, 2048, sdfile);
  }
  else
  {
    discoffset = rootdir;
    dvd_read (&readbuffer, 2048, discoffset);
  }

  /* determine file type */
  if (!IsZipFile ((char *) readbuffer))
  {
    /* go back to file start */
    if (UseSDCARD)
	  {
      fseek(sdfile, 0, SEEK_SET);
    }

    /* read data chunks */
    for ( i = 0; i < blocks; i++ )
	  {
	    if (UseSDCARD)
      {
        fread(readbuffer, 1, 2048, sdfile);
      }
	    else
      {
        dvd_read(readbuffer, 2048, discoffset);
		    discoffset += 2048;
	    }
	
      memcpy (buffer + readoffset, readbuffer, 2048);
	    readoffset += 2048;
    }

	  /* final read */ 
	  i = rootdirlength % 2048;
    if (i)
    {
      if (UseSDCARD) fread(readbuffer, 1, i, sdfile);
      else dvd_read (readbuffer, 2048, discoffset);
      memcpy (buffer + readoffset, readbuffer, i);
    }
  }
  else
  {
    /* unzip file */
    return UnZipBuffer (buffer, discoffset, rootdirlength, UseSDCARD);
  }
  
  /* close SD file */
  if (UseSDCARD) fclose(sdfile);

  return rootdirlength;
}

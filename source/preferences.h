/*
 * file_fat.c
 * 
 *   Handle Preferences via XML
 *
 *   code by Talantyyr (2010) 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************************/
 
#define APPFOLDER      "hugo"
#define PREF_FILE_NAME "settings.xml"
 
int saveSettings();
int load_settings();
int update_settings();
extern void WaitPrompt (char *msg);




 
struct SHugoSettings{

    char    ip[80];
    char    user[20];
    char    pwd[20];
    char    share[20];
	
    int     aspect; // 0 - automatic, 1 - NTSC (480i), 2 - Progressive (480p), 3 - PAL (50Hz), 4 - PAL (60Hz)
    int     render;         // 0 - original, 1 - filtered, 2 - unfiltered
  
    int     language;
};

extern struct SHugoSettings HugoSettings;
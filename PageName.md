--[Introduction ](.md)-----------------------------------------------------------

Hu-Go! GC is a PC Engine / TurboGrafx-16 emulator, now ported to the Nintendo
Gamecube. Using this emulator you'll be able to enjoy the first 16-bit gaming
experience all over again!

--[Features ](.md)---------------------------------------------------------------

  * Completely Re-Written Sound Engine
  * 16-bit Stereo 32Khz Sound
  * Load ROMS up to 2.5Mb
  * Battery RAM (WRAM) support (save & load)
  * DVD ROM Loading (GC version only, up to 4.7GB support for Wii users)
  * SDCARD ROM Loading (with subdirectory support)
  * Zipped Roms Support (Single ROM per archive)
  * ISO/BIN support (no sound)
  * Save & Load WRAM files to/from Memory Card & SDCARD
  * Horizontal Resolutions up to 320
  * Wiimote/Nunchuk/Classic controller support (Wii only)
  * SMB Support (Wii only)

--[Installation ](.md)-----------------------------------------------------------

The emulator is provided as a .dol which is a Gamecube Executable. You only need to load and run
the DOL on your GC or WII (Gamecube Pad needed) using various methods (Bootable DVD, SDLOAD,...)
If you have no idea on how to load&run a DOL, please go here on follow the available guides:
http://modyawii.tehskeen.com/  (Booting Homebrew Section)

.Wii versions is also provided: they have been compiled to run in Wii mode and should be loaded
using your prefered method (Twilight hack,...). This version provides extra features such as native SD
slot access and Wiimote support.

SDCARD users should create a directory named "hugo" at the root of the SDCARD.
Inside this directory, you have to create then a subdirectory named "roms" to put all your roms.
You should also create a subdirectory named "saves" where SRAM and FreezeState files will be saved.

If using a DVD to load the roms, the format of the image you burned must be ISO9960
or you won't be able to read from it. The maximal readable size is 1.35GB for Gamecube users
and 4.7GB for Wii users.

.Please note that only the Gamecube version of this emulator has DVD support, and you need a mod-chipped console to run DVD-R.


IMPORTANT: When putting roms either on DVD or SDCARD, it is recommended to use subdirectories as there is
> a limit of 1000 files per directory.

--[SMB Settings ](.md)-----------------------------------------------------------

If you want to use SMB, open hugo/hugo.xml and look for these Lines:



&lt;setting name="ip" value="" description="Share Computer IP" /&gt;




&lt;setting name="share" value="" description="Share Name" /&gt;




&lt;setting name="user" value="" description="Share Username" /&gt;




&lt;setting name="pwd" value="" description="Share Password" /&gt;



Enter your Configuration data in the value="" Field.

E.g like this:



&lt;setting name="ip" value="192.168.0.1" description="Share Computer IP" /&gt;




&lt;setting name="share" value="PCEngine" description="Share Name" /&gt;




&lt;setting name="user" value="wii" description="Share Username" /&gt;




&lt;setting name="pwd" value="wii" description="Share Password" /&gt;



--[Controls ](.md)---------------------------------------------------------------

The Gamecube joypad is mapped as follows:

> Button A        PCE Button 1
> Button B        PCE Button 2
> Button X        PCE Turbo 1
> Button Y        PCE Turbo 2
> Trigger Z       PCE Select
> Start           PCE Run
> L trigger	  Menu

When no expansion controller is connected, the Wiimote is used horizontally
and mapped as follows:

> Button 1        PCE Button 1
> Button 2        PCE Button 2
> Button A        PCE Turbo 1
> Button B        PCE Turbo 2
> MINUS           PCE Run
> PLUS		  PCE Select
> HOME		  Menu

When a Nunchuk is connected, the Wiimote is used vertically and mapped as follows:

> Button A        PCE Button 1
> Button B        PCE Button 2
> Z Trigger       PCE Turbo 1
> C Trigger       PCE Turbo 2
> MINUS           PCE Run
> PLUS		  PCE Select
> HOME		  Menu

When a Classic Controller is connected, the Wiimote is not used and the controller is
mapped as follows:

> Button B        PCE Button 1
> Button A        PCE Button 2
> Button Y        PCE Turbo 1
> Button X        PCE Turbo 2
> MINUS           PCE Run
> PLUS		  PCE Select
> HOME		  Menu



--[Main Menu ](.md)---------------------------------------------------------------

> You'll start off with the main introduction screen and after pressing "A"
> you will be at the main menu. Note that at anytime during gameplay you can
> return to the main menu by tapping on that little Z button (you know, the
> one on your controller).

> Of course it's a menu so you have yourself a few things to choose from.
> So here's a list and what they do.


> PLAY GAME:    Takes you into or back to the game.
> 
---



> HARD RESET:    Reset emulator
> 
---



> LOAD NEW GAME:
> 
---


> . Load from DVD: DVD must be ISO9660 (Gamecube Mode version only)
> . Load from SDCARD: You have to use a SD adapter in a MC Slot (Gamecube Mode version)
> > or a SDCARD in the native Wii SD slot (Wii mode version)


> When using SDCARD, roms should be initially placed in the /hugo/roms/ subdirectory

> In both cases, the maximum number of files per directory is 1000
> It is recommended to use subdirectories.
> Pressing B will make you going up one directory while navigating.


> EMULATOR OPTIONS:
> 
---


> Aspect let you choose the Display Aspect Ratio:
> . ORIGINAL mode automatically set the correct aspect ratio exactly
> as if you connected a real Pc-Engine on your TV.
> . STRETCH mode will stretch the display vertically to fit the whole
> screen area

> Render let you choose the Display Rendering mode:
> . ORIGINAL let you use the original PC-Engine rendering mode (240p):
> In this mode, games should look exactly as they did on the real hardware.
> Be aware that this mode might not being compatible with HDTV and the component cable.

> .BILINEAR vertically scales (using hardware filtering features) the original display
> to a 480 lines interlaced display.
> In this mode, because of the higher resolution, games generally look better than on
> the real hardware but some artifacts might appear during intensive and fast action.

> .PROGRESS swith the rendering to Progressive Video Mode (480p),
> only set when component cable is detected.

> SMB Settings shows your current Settings from hugo/hugo.xml
> > In upcoming versions, you'll be able to enter values directly from the GUI, using
> > an OnScreen Keyboard. Currently you're only able to check your SMB Settings.




> WRAM MANAGER:
> 
---


> This let you save & load the content of the system internal battery which is used
> by some games to save your progress.

> You can now choose the device type and location.
> Be sure to set this according to your system configuration before saving/loading files.

> . Device: Let you choose the device to use: SDCARD or MCARDA or MCARDB

> IMPORTANT:

  1. when using NGC Memory Card in SLOTA, some mounting errors may occur. In this case,
> > remove and insert the Memory Card again before trying to save/load anything.


> 2/ when using SDCARD, the directory /hugo/saves is automatically created



**(Gamecube Mode version only)

---**

> STOP DVD MOTOR :
> 
---

> > Stop the DVD motor and the disc from spinning during playtime


> SYSTEM REBOOT:
> 
---

> > This will reboot the system


> SD/PSO LOADER:
> 
---

> > Return to the SD or PSO Loader


**(Wii Mode version only)

---**


> SYSTEM MENU:
> 
---

> This will return to the Wii System menu

> RETURN TO LOADER:
> 
---

> Return to the loader (TP Loader or HB channel)



--[Develloper Notes ](.md)----------------------

According to the GNU status of this project, the sourcecode MUST be accessible with any binary releases you made.
To recompile the sourcecode, you will need to have installed:
> . last DevkitPPC environment
> . last compiled libOGC sources

If you have no idea on how to compile DOLs , please refer to this thread:
http://www.tehskeen.com/forums/showthread.php?t=2968.


--[Credits ](.md)----------------------------------------------------------------

Hu-Go! 2.12       Zeograd (http://www.zeograd.com)
PC2E              Ki
TGEMU             Charles MacDonald (http://cgfm2.emuviews.com)

PSG Info          Paul Clifford (http://www.magicengine.com/mkit/doc.html)
SN76489           John Kortink

Font Engine       Qoob
DVD Magic         Ninjamod Team
GX Engine         gc-linux team (http://www.gc-linux.org)
libOGC            shagkur
NGC porting code  softdev
additional code	  eke-eke
LibOGC		  shagkur & wintermute

--[Thanks ](.md)-----------------------------------------------------------------

The Hu-Go! logo, used on screen and for Save icons, are used with permission
from Zeograd.
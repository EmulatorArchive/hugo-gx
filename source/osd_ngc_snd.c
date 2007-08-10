/****************************************************************************
 * Hu-Go! Nintendo Gamecube
 *
 * Sound Functions
 ****************************************************************************/
#include "cleantyp.h"
#include "sys_snd.h"
#include "sound.h"
#include "osd_ngc_mix.h"

extern void InitGCAudio();

void osd_snd_set_volume(UChar v)
{
}

/****************************************************************************
 * Sound is taken outside of the Hu-Go! SDL driver and is performed
 * entirely by ngc_mix.c
 *
 * This function kicks off the dma Audio system.
 ****************************************************************************/
int osd_snd_init_sound(void)
{

	/*** Required values for Nintendo Gamecube ***/
	host.sound.stereo = 1;
	host.sound.sample_size = 512;
	host.sound.freq = 32000;
	host.sound.signed_sound = 1;

	InitGCAudio();
	InitPSG( host.sound.freq );
	return 1;
}

void osd_snd_trash_sound(void)
{
}


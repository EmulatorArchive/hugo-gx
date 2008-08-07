/****************************************************************************
 * Hu-Go! Nintendo Gamecube
 *
 * Keyboard functions
 ****************************************************************************/
#include "pce.h"
extern unsigned char cart_reload;
extern int GetJoys (int joy);

int
osd_keyboard (void)
{
	int i;

	for ( i = 0; i < 3; i++ )
		io.JOY[i] = GetJoys(i);

	return cart_reload;
}

/*****************************************************************************
 * Main joystick values
 *****************************************************************************/
int osd_init_input()
{
	return 0;
}

void osd_shutdown_input(void)
{
}

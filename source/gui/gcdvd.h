/****************************************************************************
 * dvd.h
 *
 * Supporting DVD functions
 ****************************************************************************/

/*** Common DVD readbuffer ***/
extern unsigned char readbuffer[2048];
extern unsigned int dvd_read(void *dst, unsigned int len, unsigned int offset);

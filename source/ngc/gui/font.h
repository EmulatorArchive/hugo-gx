/****************************************************************************
 * IPL_FONT HEADER
 ****************************************************************************/
void init_font(void);
void write_font(int x, int y, char *string);
void writex(int x, int y, int sx, int sy, char *string, unsigned int *lookup);
void write_centre( int y, char *string);
void write_centre_hi(int y, char *string);
extern int font_height;
extern int font_size[256];
extern u16 back_framewidth;

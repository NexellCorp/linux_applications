#include <stdio.h>

#ifndef __FB_DRAW__
#define	__FB_DRAW__

void 	*load_bmp2rgb  (const char *name,
						unsigned long framebase, int xresol, int yresol,
						int pixelbyte);

void 	*load_bmp2uyuv (const char *name,
						unsigned long framebase, int xresol, int yresol,
						int pixelbyte);

void   	unload_bmp	(void *buffer);

void 	fill_rgb 	(unsigned long framebase,
						int xresol, int yresol, int pixelbyte,
						int startx, int starty, int width, int height,
						unsigned int color);

void 	fill_uyuv	(unsigned long framebase,
						int xresol, int yresol, int pixelbyte,
						int startx, int starty, int width, int height,
						unsigned int color);

int	 	get_bmp_info(const char *name, int *width, int *height, int *bitcount);

void 	copy_uyuv	(unsigned long framebase,
						int xresol, int yresol, int pixelbyte,
						int startx, int starty, int image_width, int image_height,
						int image_pixel, unsigned long image_base);

void 	copy_rgb 	(unsigned long framebase,
						int xresol, int yresol, int pixelbyte,
						int startx, int starty, int image_width, int image_height,
						int image_pixel, unsigned long image_base);

#endif

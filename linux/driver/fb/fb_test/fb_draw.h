#include <stdio.h>

#ifndef __FB_DRAW__
#define	__FB_DRAW__

#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#define UNUSED(x) (void)(x)

#define __ALIGN(x, a)	(((x) + (a)) & ~(a))
#define FB_ALIGN(v, a) 	__ALIGN(v, (a - 1))


void fill_rgb(unsigned long framebase,
			int xresol, int yresol, int pixelbyte,
			int startx, int starty, int width, int height,
			unsigned int color);

void fill_uyuv(unsigned long framebase,
			int xresol, int yresol, int pixelbyte,
			int startx, int starty, int width, int height,
			unsigned int color);

void *load_bmp2rgb(const char *name,
			unsigned long framebase, int xresol, int yresol,
			int pixelbyte);
void *load_bmp2uyuv (const char *name,
			unsigned long framebase, int xresol, int yresol,
			int pixelbyte);
void unload_bmp	(void *buffer);

int	get_bmp_info(const char *name, int *width, int *height, int *bitcount);

void copy_uyuv(unsigned long framebase,
			int xresol, int yresol, int pixelbyte,
			int startx, int starty, int image_width, int image_height,
			int image_pixel, unsigned long image_base);

void copy_rgb(unsigned long framebase,
			int xresol, int yresol, int pixelbyte,
			int startx, int starty, int image_width, int image_height,
			int image_pixel, unsigned long image_base);

enum lcd_fonts {
	eng_8x16,
};

int draw_text(const char *string,
			int sx, int sy, int hscale, int vscale,
			int alpha, unsigned int text_color,
			unsigned int back_color,
			int font_type,
			unsigned long framebase,
			int xresol, int yresol, int pixelbyte);

#endif

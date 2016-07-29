#include <stdio.h>
#include <stdlib.h>		/* malloc */
#include <string.h> 	/* strerror */
#include <errno.h> 		/* errno */
#include <fcntl.h> 		/* for O_RDWR */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fb_draw.h"
#include "font8x16.h"

#if 0
#define	pr_debug(msg...)	printf(msg)
#else
#define	pr_debug(msg...)
#endif

#define	DEBUG_BMPHEADER	0

/*------------------------------------------------------------------------------
 *	BMP type define
 */
#ifndef u32
typedef unsigned int u32;
#endif	/* u32 */
#ifndef u16
typedef unsigned short u16;
#endif	/* u16 */
#ifndef u8
typedef unsigned char u8;
#endif	/* u8  */

typedef struct __BITMAPFILEHEADER {
/* 	u16 	bfType; */
  	u32   	size;
  	u16 	reserved1;
	u16 	reserved2;
  	u32 	off_bits;
} BITMAPFILEHEADER;

typedef struct __BITMAPINFOHEADER {
  	u32 size;
  	u32 width;
  	u32 height;
  	u16 planes;
  	u16	bit_count;
  	u32 compression;
  	u32 size_image;
  	u32 x_pels_per_meter;
  	u32 y_pels_per_meter;
  	u32 clr_used;
  	u32 clr_important;
} BITMAPINFOHEADER;

static u8 dither_pattern6[4][2][2] = {
	{ {1, 0}, {0, 0} },
	{ {1, 0}, {0, 1} },
	{ {1, 1}, {0, 1} },
	{ {1, 1}, {1, 1} },
};

static u8 dither_pattern5[8][3][3] = {
	{ {0, 0, 0}, {0, 1, 0}, {0, 0, 0} },
	{ {0, 0, 0}, {0, 1, 0}, {1, 0, 0} },
	{ {1, 0, 0}, {0, 1, 0}, {1, 0, 0} },
	{ {1, 0, 1}, {0, 1, 0}, {1, 0, 0} },
	{ {1, 0, 1}, {0, 1, 0}, {1, 0, 1} },
	{ {1, 0, 1}, {1, 1, 0}, {1, 0, 1} },
	{ {1, 1, 1}, {1, 1, 0}, {1, 0, 1} },
	{ {1, 1, 1}, {1, 1, 0}, {1, 1, 1} }
};

#define	RGB888TO565(col)	((((col>>16)&0xFF)&0xF8)<<8) |	\
		((((col>>8)&0xFF)&0xFC)<<3) | ((((col>>0 )&0xFF)&0xF8)>>3)

#define	RGB555TO565(col)	(((col>>10)&0x1F) << 11) |	\
		(((col>> 5)&0x1F) << 6) | ((col<< 0)&0x1F)

static void put_pixel_888to565(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(u16 *)(base + (ypos * width + xpos) * 2) = (u16)RGB888TO565(color);
}

static void put_pixel_555to565(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(u16 *)(base + (ypos * width + xpos) * 2) = (u16)RGB555TO565(color);
}

#if 0
static void put_pixel_565to565(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(u16 *)(base + (ypos * width + xpos) * 2) = (u16)color & 0xFFFF;
}
#endif

static void put_pixel_888to888(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(u8 *)(base + (ypos * width + xpos) * 3 + 0) = ((color>> 0)&0xFF);
	*(u8 *)(base + (ypos * width + xpos) * 3 + 1) = ((color>> 8)&0xFF);
	*(u8 *)(base + (ypos * width + xpos) * 3 + 2) = ((color>>16)&0xFF);
}

static void put_pixel_565to888(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(u8 *)(base + (ypos * width + xpos) * 3 + 0) =
		(((color >> 0 ) << 3) & 0xf8) | (((color >> 0 ) >> 2) & 0x7);
	*(u8 *)(base + (ypos * width + xpos) * 3 + 1) =
		(((color >> 5 ) << 2) & 0xfc) | (((color >> 5 ) >> 4) & 0x3);
	*(u8 *)(base + (ypos * width + xpos) * 3 + 2) =
	(((color >> 11) << 3) & 0xf8) | (((color >> 11) >> 2) & 0x7);
}

static void put_pixel_888to8888(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(u32 *)(base + (ypos * width + xpos) * 4) = (u32)(color | (0xFF<<24));
}

static void put_pixel_565to8888(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(u8 *)(base + (ypos * width + xpos) * 4 + 0) =
		(((color >> 0 ) << 3) & 0xf8) | (((color >> 0 ) >> 2) & 0x7);
	*(u8 *)(base + (ypos * width + xpos) * 4 + 1) =
		(((color >> 5 ) << 2) & 0xfc) | (((color >> 5 ) >> 4) & 0x3);
	*(u8 *)(base + (ypos * width + xpos) * 4 + 2) =
		(((color >> 11) << 3) & 0xf8) | (((color >> 11) >> 2) & 0x7);
	*(u8 *)(base + (ypos * width + xpos) * 4 + 3) = 0xFF; // No Alpha
}

#define RGB565_ALPHA_MASK	0xF7DE

static void alpha_pixel_888to565(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	*(unsigned short*)((ulong)(base + (ypos * width + xpos) * 2)) =
		(*(unsigned short*)((ulong)(base + (ypos * width + xpos) * 2)) &
		RGB565_ALPHA_MASK>>1) | (unsigned short)RGB888TO565(color);
}

static void alpha_pixel_888to888(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	base = base + (ypos * width + xpos) * 3;
	*(u8 *)((ulong)(base)) = (*(u8 *)((ulong)(base))>>1) | ((color>> 0)&0xFF);	base++;
	*(u8 *)((ulong)(base)) = (*(u8 *)((ulong)(base))>>1) | ((color>> 8)&0xFF);	base++;
	*(u8 *)((ulong)(base)) = (*(u8 *)((ulong)(base))>>1) | ((color>>16)&0xFF);
}

static void alpha_pixel_888to8888(unsigned long base,
			int xpos, int ypos, int width, int height, unsigned int color)
{
	UNUSED(height);
	base = base + (ypos * width + xpos) * 4;
	*(unsigned int*)((ulong)base) = (0xFF000000) | (color & 0xFFFFFF) |
					(*(unsigned int*)((ulong)base));
}

typedef void (*drawpixel_t)(unsigned long, int, int, int, int, unsigned int);

drawpixel_t put_pixel_fns[] = {
	put_pixel_555to565,
	put_pixel_565to888,
	put_pixel_565to8888,
	put_pixel_888to565,
	put_pixel_888to888,
	put_pixel_888to8888,
};

drawpixel_t alpha_pixel_fns[] = {
	alpha_pixel_888to565,
	alpha_pixel_888to888,
	alpha_pixel_888to8888,
};

static inline void put_pixel_uyvy(unsigned long base, u32 xpos, u32 ypos,
			u8 *py, u8 *pu, u8 *pv, u32 stride)
{
	/* UY0VY1 */
	*(u8 *)(base + ((ypos+0) * stride) + xpos + 0) = (u8)((pu[0] + pu[1]) >> 1);
	*(u8 *)(base + ((ypos+0) * stride) + xpos + 1) = (u8)(py[0]);
	*(u8 *)(base + ((ypos+0) * stride) + xpos + 2) = (u8)((pv[0] + pv[1]) >> 1);
	*(u8 *)(base + ((ypos+0) * stride) + xpos + 3) = (u8)(py[1]);

	/* UY0VY1 */
	*(u8 *)(base + ((ypos+1) * stride) + xpos + 0) = (u8)((pu[2] + pu[3]) >> 1);
	*(u8 *)(base + ((ypos+1) * stride) + xpos + 1) = (u8)(py[2]);
	*(u8 *)(base + ((ypos+1) * stride) + xpos + 2) = (u8)((pv[2] + pv[3]) >> 1);
	*(u8 *)(base + ((ypos+1) * stride) + xpos + 3) = (u8)(py[3]);
}

static void	rgb_to_yuv(u8 r, u8 g, u8 b, u8 *py, u8 *pu, u8 *pv)
{
#if 1
#if 1
	/* ypos, C = 0 ~ 255 */
	*py = (u8)(((  66 * (int)r + 129 * (int)g +  25 * (int)b + 128) >> 8) +  16);
	*pu = (u8)((( -38 * (int)r -  74 * (int)g + 112 * (int)b + 128) >> 8) + 128);
	*pv = (u8)((( 112 * (int)r -  94 * (int)g -  18 * (int)b + 128) >> 8) + 128);
#else
	*py = (u8)(( 0.299*r) + (0.587*g) + (0.114*b)) + 16;
	*pu = (u8)((-0.148*r) - (0.291*g) + (0.439*b)) + 128;
	*pv = (u8)(( 0.439*r) - (0.368*g) - (0.071*b)) + 128;
#endif
#else
	/* Y = 16 ~ 235 */
	/* C = 16 ~ 240 */
	*py = (u8)( 0.299*r + 0.587*g + 0.114*b);
	/* cbcr range = -128~127, spica support up to 0, so add 128  */
	*pu = (u8)(-0.147*r - 0.289*g + 0.436*b) + 128;
	/* cbcr range = -128~127, spica support up to 0, so add 128  */
	*pv = (u8)( 0.615*r - 0.515*g - 0.100*b) + 128;
#endif
}

//------------------------------------------------------------------------------
void * load_bmp2rgb(const char *name,
			unsigned long framebase, int xresol, int yresol,
			int pixelbyte)
{
	u16	BMP_ID;
	BITMAPFILEHEADER  bmp_file;
	BITMAPINFOHEADER  bmp_info;

	u8 *mapptr  = NULL;
	int bmp_pixelbyte;

	int s_sx, s_sy;
	int b_sx, b_sy, b_ex, b_ey;
	int lx, ly, bx, by;

	u8 *pixptr;
	u32 color;
	u32 bmp_pitch;

	struct stat fsta;

	int    fd;
	char   path[256] = { 0, };
	u8  *  buffer;
	unsigned long  length;
	int ret;

	drawpixel_t draw_fn;

	strcpy(path, name);
   	if (0 > stat(path, &fsta)) {
      	printf("Fail: %s stat: %s\n", path, strerror(errno));
      	return 0;
   }

	fd = open(path, O_RDONLY);
	if (0 > fd) {
		printf("Fail: %s open: %s\n", path, strerror(errno));
		return 0;
	}

	/* Check BMP file type. */
	ret = read(fd, (void*)&BMP_ID, 2);
	if (0 == ret || (BMP_ID != 0x4D42 && BMP_ID != 0x4d42)) {
		printf("Fail: %s Unknonw BMP (type:0x%x, len=%d) ...\n",
			path, BMP_ID, (int)fsta.st_size);
		return NULL;
	}

	/* Get BMP header */
	ret = read(fd, (void*)&bmp_file, sizeof(BITMAPFILEHEADER));
	ret = read(fd, (void*)&bmp_info, sizeof(BITMAPINFOHEADER));

#if (DEBUG_BMPHEADER == 1)
	printf("\nBMP File Heade \r\n");
	printf("type	  : 0x%x \r\n", BMP_ID);
	printf("size	  : %d   \r\n", bmp_file.size);
	printf("offs	  : %d   \r\n", bmp_file.off_bits);
	printf("\nBMP Info Header  \r\n");
	printf("size      : %d\r\n", bmp_info.size);
	printf("width     : %d\r\n", bmp_info.width);
	printf("height    : %d\r\n", bmp_info.height);
	printf("planes    : %d\r\n", bmp_info.planes);
	printf("bitcount  : %d\r\n", bmp_info.bit_count);
	printf("compress  : %d\r\n", bmp_info.compression);
	printf("size image: %d\r\n", bmp_info.size_image);
	printf("x pels    : %d\r\n", bmp_info.x_pels_per_meter);
	printf("y pels    : %d\r\n", bmp_info.y_pels_per_meter);
	printf("clr used  : %d\r\n", bmp_info.clr_used);
	printf("clr import: %d\r\n", bmp_info.clr_important);
	printf("\r\n");
#endif
	bmp_pixelbyte = bmp_info.bit_count/8;

	/* a line size of BMP must be aligned on DWORD boundary. */
	length = ((((bmp_info.width * 3) + 3) / 4) * 4) * bmp_info.height;

	/* Load bmp to memory */
	buffer = (u8 *)malloc(length);
	if (! buffer) {
		printf("error, allocate memory (size: %ld)...\n", length);
		close(fd);
		return 0;
	}

	lseek(fd, (int)(bmp_file.off_bits), SEEK_SET);	/* BMP file end point. */
	ret = read(fd, buffer, length);
	close(fd);

	/* set bmp base */
	mapptr = (u8*)(buffer);
	s_sx = 0, s_sy = 0;
	b_sx = 0, b_sy = 0;
	b_ex = bmp_info.width-1, b_ey = bmp_info.height-1;

	printf("DONE: BMP %d by %d (%dbpp), len=%d \r\n",
		bmp_info.width, bmp_info.height, bmp_pixelbyte, bmp_file.size);
	printf("DRAW: on 0x%08lx \r\n", framebase);

	if ((int)bmp_info.width  > xresol) {
		b_sx = (bmp_info.width - xresol)/2;
		b_ex = b_sx + xresol;
	} else
	if ((int)bmp_info.width  < xresol)	{
		s_sx += (xresol- bmp_info.width)/2;
	}

	if ((int)bmp_info.height > yresol)	{
		b_sy = (bmp_info.height - yresol)/2;
		b_ey = b_sy + yresol;
	} else
	if ((int)bmp_info.height < yresol) {
		s_sy += (yresol- bmp_info.height)/2;
	}

	/*
	 * Select put pixel function
	 */
	switch (bmp_pixelbyte) {
	case 2:	draw_fn = put_pixel_fns[0 + pixelbyte-2];	break;	/* 565 to 565/888 */
	case 3: draw_fn = put_pixel_fns[3 + pixelbyte-2];	break;	/* 888 to 565/888 */
	default:
		printf("Not support BMP BitPerPixel (%d) ...\r\n", bmp_pixelbyte);
		return NULL;
	}

	/*
	 * BMP stride ,
	 * a line size of BMP must be aligned on DWORD boundary.
	 */
	bmp_pitch = (((bmp_info.width*3)+3)/4)*4;

	/*
	 * Draw 16 BitperPixel image on the frame buffer base.
	 * RGB555
	 */
	if (bmp_pixelbyte == 2 && bmp_info.compression == 0x00000000) {
		for(ly = s_sy, by = b_ey; by>=b_sy; ly++, by--) {
			for(lx = s_sx, bx = b_sx; bx<=b_ex; lx++, bx++) {
				color = *(u16*)(mapptr + (by * bmp_pitch) + (bx * bmp_pixelbyte));
				color = (u16)RGB555TO565(color);
				draw_fn(framebase, lx, ly, xresol, yresol, color);
			}
		}
	}

	/*
	 * Draw 16 BitperPixel image on the frame buffer base.
	 * RGB565
	 */
	if (bmp_pixelbyte == 2 && bmp_info.compression == 0x00000003) {
		for(ly = s_sy, by = b_ey; by>=b_sy; ly++, by--) {
			for(lx = s_sx, bx = b_sx; bx<=b_ex; lx++, bx++) {
				color = *(u16*)(mapptr + (by * bmp_pitch) + (bx * bmp_pixelbyte));
				draw_fn(framebase, lx, ly, xresol, yresol, color);
			}
		}
	}

	/*
	 * Draw 24 BitperPixel image on the frame buffer base.
	 */
	if (bmp_pixelbyte == 3) {
		u32	red, green, blue;

		for(ly = s_sy, by = b_ey; by>=b_sy; ly++, by--) {
			for(lx = s_sx, bx = b_sx; bx<=b_ex; lx++, bx++) {

				pixptr   = (u8*)(mapptr + (by * bmp_pitch) + (bx * bmp_pixelbyte));
				red   = *(pixptr+2);
				green = *(pixptr+1);
				blue  = *(pixptr+0);
				if (2 == pixelbyte) {
					red   = dither_pattern5[red   & 0x7][lx%3][ly%3] + (red  >>3);
					red   = (red  >31) ? 31: red;
					green = dither_pattern6[green & 0x3][lx%2][ly%2] + (green>>2);
					green = (green>63) ? 63 : green;
					blue  = dither_pattern5[blue  & 0x7][lx%3][ly%3] + (blue >>3);
					blue  = (blue >31) ? 31: blue;
					color	= (red<<11) | (green<<5) | (blue);
					*(u16*)(framebase + (ly * xresol + lx) * 2) = (u16)color;
				} else	 {
					color = ((red&0xFF)<<16) | ((green&0xFF)<<8) | (blue&0xFF);
					draw_fn(framebase, lx, ly, xresol, yresol, color);
				}
			}
		}
	}

	return (void*)buffer;
}

void * load_bmp2uyuv(const char *name,
					unsigned long framebase, int xresol, int yresol,
					int pixelbyte)
{
	BITMAPFILEHEADER  bmp_file;
	BITMAPINFOHEADER  bmp_info;
	u16	BMP_ID;
	u8 *mapptr = NULL;
	int bmp_pixelbyte;

	int s_sx, s_sy;
	int b_sx, b_sy, b_ex, b_ey;
	int lx, ly, bx, by;

	u8 *pixptr;

	struct stat fsta;

	int fd;
	char  path[256] = { 0, };
	u8  * buffer;
	unsigned long  length;
	int ret;

	strcpy(path, name);
   	if (0 > stat(path, &fsta)) {
      	printf("Fail: %s stat: %s\n", path, strerror(errno));
      	return 0;
   }

	fd = open(path, O_RDONLY);
	if (0 > fd) {
		printf("Fail: %s open: %s\n", path, strerror(errno));
		return 0;
	}

	// Check BMP file type.
	ret = read(fd, (void*)&BMP_ID, 2);
	if (ret > 0 || BMP_ID != 0x4D42) {
		printf("Fail: %s Unknonw BMP (type:0x%x, len=%d) ...\n",
			path, BMP_ID, (int)fsta.st_size);
		return NULL;
	}

	/* Get BMP header */
	ret = read(fd, (void*)&bmp_file, sizeof(BITMAPFILEHEADER));
	ret = read(fd, (void*)&bmp_info, sizeof(BITMAPINFOHEADER));

#if (DEBUG_BMPHEADER == 1)
	printf("\nBMP File Heade \r\n");
	printf("Type	: 0x%x \r\n", BMP_ID);
	printf("Size	: %d   \r\n", bmp_file.size);
	printf("Offs	: %d   \r\n", bmp_file.off_bits);
	printf("\nBMP Info Header  \r\n");
	printf("Size     : %d\r\n", bmp_info.size);
	printf("width    : %d\r\n", bmp_info.width);
	printf("height   : %d\r\n", bmp_info.height);
	printf("Planes   : %d\r\n", bmp_info.planes);
	printf("BitCount : %d\r\n", bmp_info.bit_count);
	printf("Compress : %d\r\n", bmp_info.compression);
	printf("SizeImage: %d\r\n", bmp_info.size_image);
	printf("XPels    : %d\r\n", bmp_info.x_pels_per_meter);
	printf("YPels    : %d\r\n", bmp_info.y_pels_per_meter);
	printf("ClrUsed  : %d\r\n", bmp_info.clr_used);
	printf("ClrImport: %d\r\n", bmp_info.clr_important);
	printf("\r\n");
#endif
	bmp_pixelbyte = bmp_info.bit_count/8;

	/* a line size of BMP must be aligned on DWORD boundary. */
	length = ((((bmp_info.width * 3) + 3) / 4) * 4) * bmp_info.height;

	/* Load bmp to memory */
	buffer = (u8 *)malloc(length);
	if (! buffer) {
		printf("error, allocate memory (size: %ld)...\n", length);
		close(fd);
		return 0;
	}

	lseek(fd, (int)(bmp_file.off_bits), SEEK_SET);	/* BMP file end point. */
	ret = read(fd, buffer, length);
	close(fd);

	/* set bmp base */
	mapptr = (u8*)(buffer);
	s_sx = 0, s_sy = 0;
	b_sx = 0, b_sy = 0;
	b_ex = bmp_info.width-1, b_ey = bmp_info.height-1;

	printf("DONE: BMP %d by %d (%dbpp), len=%d \r\n",
		bmp_info.width, bmp_info.height, bmp_pixelbyte, bmp_file.size);
	printf("DRAW: on 0x%08lx \r\n", framebase);

	if ((int)bmp_info.width  > xresol)	{
		b_sx = (bmp_info.width - xresol)/2;
		b_ex = b_sx + xresol;
	} else
	if ((int)bmp_info.width  < xresol)	{
		s_sx += (xresol- bmp_info.width)/2;
		s_sx  = ((s_sx + 3)/4) *4;	/* align 4 pixel for YUYV */
	}

	if ((int)bmp_info.height > yresol)	{
		b_sy = (bmp_info.height - yresol)/2;
		b_ey = b_sy + yresol;
	} else
	if ((int)bmp_info.height < yresol)	{
		s_sy += (yresol- bmp_info.height)/2;
	}

	/*
	 * Draw 16 BitperPixel image on the frame buffer base.
	 * RGB555
	 */
	if (bmp_pixelbyte == 2 && bmp_info.compression == 0x00000000)
		printf("Fail: %s not support %d Bpp\n", path, bmp_pixelbyte*8);

	/*
	 * Draw 16 BitperPixel image on the frame buffer base.
	 * RGB565
	 */
	if (bmp_pixelbyte == 2 && bmp_info.compression == 0x00000003)
		printf("Fail: %s not support %d Bpp\n", path, bmp_pixelbyte*8);

	/* Draw 24 BitperPixel image on the UYVY frame buffer */
	if (bmp_pixelbyte == 3) {
		u8	r, g, b, Y[2][2], U[2][2], V[2][2];
		u32 lcd_pitch = xresol * pixelbyte;			/* LCD stride */
		u32 bmp_pitch = (((bmp_info.width * 3) + 3) / 4) * 4;
		/* BMP stride ,
		 a line size of BMP must be aligned on DWORD boundary. */
		for(ly = s_sy, by = b_ey; by > b_sy; ly += 2, by -= 2) {
			for(lx = s_sx*pixelbyte, bx = b_sx; b_ex >= bx; lx+=(pixelbyte + 2), bx+=2) {
				pixptr = (u8*)(mapptr + (by * bmp_pitch) + (bx * bmp_pixelbyte));

				r  = *(pixptr + 2);
				g  = *(pixptr + 1);
				b  = *(pixptr + 0);
				rgb_to_yuv((int)r, (int)g, (int)b, &Y[0][0], &U[0][0], &V[0][0] );

				r  = *(pixptr + 5);
				g  = *(pixptr + 4);
				b  = *(pixptr + 3);
				rgb_to_yuv((int)r, (int)g, (int)b, &Y[0][1], &U[0][1], &V[0][1] );

				r  = *(pixptr + 2 - bmp_pitch);
				g  = *(pixptr + 1 - bmp_pitch);
				b  = *(pixptr + 0 - bmp_pitch);
				rgb_to_yuv((int)r, (int)g, (int)b, &Y[1][0], &U[1][0], &V[1][0] );

				r  = *(pixptr + 5 - bmp_pitch);
				g  = *(pixptr + 4 - bmp_pitch);
				b  = *(pixptr + 3 - bmp_pitch);
				rgb_to_yuv((int)r, (int)g, (int)b, &Y[1][1], &U[1][1], &V[1][1] );
			#if (0)
				Y[0][1] = Y[0][0], U[0][1] = U[0][0], V[0][1] = V[0][0];
				Y[1][1] = Y[1][0], U[1][1] = U[1][0], V[1][1] = V[1][0];
			#endif

				put_pixel_uyvy(framebase, (u32)lx, (u32)ly , &Y[0][0], &U[0][0], &V[0][0], lcd_pitch);
			}
		/*
		 *	printf("[%4d:%4d, %d:%d, %d]\n", ly, lx, by, bx, bmp_pitch);
		 */
		}
	}
	return (void*)buffer;
}

void   unload_bmp(void *buffer)
{
	if (buffer)
		free(buffer);
}

/*
 * COLOR: 1=White, 2=Black, 3=Red, 4=Green, 5=Blue, 6=Yellow, 7=Cyan, 8=Magenta
 */
void fill_uyuv(unsigned long framebase,
					int xresol, int yresol, int pixelbyte,
					int startx, int starty, int width, int height,
					unsigned int color)
{

	u8 YUV_WHITE[]	= { 180, 128, 128 };	/* 75% White */
	u8 YUV_BLACK[]	= {  16, 128, 128 };	/* Black */
	u8 YUV_RED[]	= {  51, 109, 212 };	/* Red */
	u8 YUV_GREEN[]	= { 133,  63,  52 };	/* Green */
	u8 YUV_BLUE[]	= {  28, 212, 120 };	/* Blue */
	u8 YUV_YELLOW[]	= { 168,  44, 136 };	/* Yellow */
	u8 YUV_CYAN[]	= { 145, 147,  44 };	/* Cyan */
	u8 YUV_MAGENTA[]= {  63, 193, 204 };	/* Magenta */

	u8 *pYUV;
	int stride = xresol * pixelbyte;
	int x, y, sx, sy, ex, ey;
	u8	Y[2][2], U[2][2], V[2][2];

	switch (color) {
	case 1: pYUV = YUV_WHITE  ; break;
	case 2: pYUV = YUV_BLACK  ; break;
	case 3: pYUV = YUV_RED    ; break;
	case 4: pYUV = YUV_GREEN  ; break;
	case 5: pYUV = YUV_BLUE   ; break;
	case 6: pYUV = YUV_YELLOW ; break;
	case 7: pYUV = YUV_CYAN   ; break;
	case 8: pYUV = YUV_MAGENTA; break;
	default:
			printf("Fail: not support yuv color (0~6)\n");
			return;
	}

	sx = startx*pixelbyte;
	sy = starty;
	ex = (startx + width)*pixelbyte;
	ey = (starty + height);

	if (startx > xresol)
		sx = xresol*pixelbyte;

	if (starty > yresol)
		sy = yresol;

	if ((startx + width) > xresol)
		ex = xresol*pixelbyte;

	if ((starty + height) > yresol)
		ey = yresol;

	pr_debug("%s: %d~%d, %d~%d (%dx%d, %dpix) 0x%08x\n",
		__func__, startx, width, starty, height, xresol, yresol, pixelbyte, color);

	for(y = sy; ey > y; y+=2) {
	for(x = sx; ex > x; x+=(pixelbyte + 2)) {
		Y[0][0] = Y[0][1] = Y[1][0] = Y[1][1] = pYUV[0];
		U[0][0] = U[0][1] = U[1][0] = U[1][1] = pYUV[1];
		V[0][0] = V[0][1] = V[1][0] = V[1][1] = pYUV[2];
		put_pixel_uyvy(framebase, (u32)x, (u32)y , &Y[0][0], &U[0][0], &V[0][0], stride);
	}
	}
}

void fill_rgb (unsigned long framebase,
					int xresol, int yresol, int pixelbyte,
					int startx, int starty, int width, int height,
					unsigned int color)
{
	int x, y, sx, sy, ex, ey;
	drawpixel_t draw_fn;
	int fn = 3 + pixelbyte - 2;

	sx = startx;
	sy = starty;
	ex = (startx + width);
	ey = (starty + height);

	if (startx > xresol)
		sx = xresol;

	if (starty > yresol)
		sy = yresol;

	if ((startx + width) > xresol)
		ex = xresol;

	if ((starty + height) > yresol)
		ey = yresol;

	pr_debug("%s: %d~%d, %d~%d (%dx%d, %dpix) 0x%08x\n",
		__func__, startx, width, starty, height, xresol, yresol, pixelbyte, color);

	/* Select put pixel function */
	if (fn > (int)ARRAY_SIZE(put_pixel_fns) - 1) {
		printf("Not support BitPerPixel (%d) ...\n", pixelbyte);
		return;
	}

	draw_fn = put_pixel_fns[3 + pixelbyte-2];

	for(y = sy; ey > y; y++)
	for(x = sx; ex > x; x++)
		draw_fn(framebase, x, y, xresol, yresol, color);
}

void copy_uyuv(unsigned long framebase,
					int xresol, int yresol, int pixelbyte,
					int startx, int starty, int image_width, int image_height,
					int image_pixel, unsigned long image_base)
{
	int stride = xresol * pixelbyte;
	int img_w, img_h;;
	int sw, sh;
	int y;

	u8 *src, *dst;

	sw = xresol - startx;
	sh = yresol - starty;

	if (sw > image_width)
		img_w = image_width * image_pixel;
	else
		img_w = sw * image_pixel;

	if (sh > image_height)
		img_h = image_height;
	else
		img_h = sh;

	/* align 4byte for YUYV */
	startx &= ~((1<<2) - 1);

	pr_debug("h=%d, w=%d\n", img_h, img_w);

	for(y = 0; img_h > y; y++) {
		dst = (u8*)(framebase + (startx * pixelbyte) + ((y + starty) * stride));
		src = (u8*)(image_base + (y * image_width * image_pixel));
		memcpy(dst, src, img_w);
	}
}

void copy_rgb (unsigned long framebase,
					int xresol, int yresol, int pixelbyte,
					int startx, int starty, int image_width, int image_height,
					int image_pixel, unsigned long image_base)
{
	int stride = xresol * pixelbyte;
	int img_w, img_h;;
	int sw, sh;
	int y;

	u8 *src, *dst;

	sw = xresol - startx;
	sh = yresol - starty;

	if (sw > image_width)
		img_w = image_width * image_pixel;
	else
		img_w = sw * image_pixel;

	if (sh > image_height)
		img_h = image_height;
	else
		img_h = sh;

	pr_debug("%s: h=%d, w=%d\n", __func__, img_h, img_w);
	for(y = 0; img_h > y; y++) {
		dst = (u8*)(framebase + (startx * pixelbyte) + ((y + starty) * stride));
		src = (u8*)(image_base + (y * image_width * image_pixel));
		memcpy(dst, src, img_w);
	/*
		printf("[%4d][%d, %d]\n", y,
			(startx * pixelbyte) + ((y + starty) * stride),
			(y * image_width * image_pixel));
	 */
	}
}

int get_bmp_info(const char *name, int *width, int *height, int *bitcount)
{
	u16					BMP_ID;
	BITMAPFILEHEADER 	bmp_file;
	BITMAPINFOHEADER 	bmp_info;

	int  fd;
	char path[256] = { 0, };
	int  bmp_w, bmp_h, bmp_bit;
	struct stat fsta;
	int ret;

	strcpy(path, name);

   	if (0 > stat(path, &fsta)) {
      	printf("fail, %s stat: %s\n", path, strerror(errno));
      	return -1;
   }

	fd = open(path, O_RDONLY);
	if (0 > fd) {
		printf("fail, %s open: %s\n", path, strerror(errno));
		return -1;
	}

	/* check bmp type */
	ret = read(fd, (void*)&BMP_ID, 2);
	if (ret > 0 || 0x4D42 != BMP_ID) {	// 0x4D42: "BM"
		printf("error, (%s) is not BMP format...\n", path);
		close(fd);
		return -1;
	}

	ret = read(fd, (void*)&bmp_file, sizeof(BITMAPFILEHEADER));
	ret = read(fd, (void*)&bmp_info, sizeof(BITMAPINFOHEADER));

	bmp_w   = (int)bmp_info.width;
	bmp_h   = (int)bmp_info.height;
	bmp_bit = (int)bmp_info.bit_count;
	if (24 != bmp_bit) {
		printf("fail, %s, It's not RGB 24bpp (%dbpp)\n", path, bmp_bit);
		close(fd);
		return -1;
	}

	if (width )	*width = bmp_w;
	if (height)	*height = bmp_h;
	if (bitcount)	*bitcount = bmp_bit;

	close(fd);

	return 0;
}

static void	text_draw_eng0816(const char *string,
				unsigned int text_color, unsigned int back_color,
				int sx, int sy, int hscale, int vscale,
				unsigned long framebase, int xresol, int yresol,
				drawpixel_t draw_fn)
{
	unsigned int xpos = sx, ypos = sy;
	unsigned char *pfont = (unsigned char *)&font8x16[(*string & 0xff)][0] ;
	unsigned char bitmask = 0;

	int	len = strlen((char *)string);
	int i, w, h;
	int xsh, ysh;
	unsigned int color;

	if (!hscale) hscale = 1;
	if (!vscale) vscale = 1;

	xsh = hscale;
	ysh = vscale;
	w = xpos + ( 8 * hscale);
	h = ypos + (16 * vscale);

	if (w > xresol)
		w = xresol;

	if (h > yresol)
		h = yresol;

	for (i = 0; i < len; i++) {
		pfont = (unsigned char *)&font8x16[(*string++ & 0xff)][0] ;

		for (sy = ypos; sy < h; sy++ ) {
			bitmask = *pfont;

			for (sx = xpos; sx < w; sx++) {
				color = bitmask & 0x80 ? text_color : back_color;

				draw_fn(framebase, sx, sy, xresol, yresol, color);

				/* Horizontal scale */
				xsh--;
				if (!xsh) {
					bitmask <<= 1;
					xsh = hscale;
				}
			}

			/* Vertical scale */
			ysh--;
			if (!ysh) {
				ysh = vscale;
				pfont++;
			}
		}
		xpos += (8 * hscale);
		w = xpos + (8 * hscale);
	}
}

int draw_text(const char *string,
				int sx, int sy, int hscale, int vscale,
				int alpha, unsigned int text_color,
				unsigned int back_color,
				int font_type,
				unsigned long framebase,
				int xresol, int yresol, int pixelbyte)
{
	drawpixel_t draw_fn = NULL;
	int fn = 3 + pixelbyte - 2;

	UNUSED(font_type);

	/* Select put pixel function */
	if (fn > (int)ARRAY_SIZE(put_pixel_fns) - 1) {
		printf("Not support BitPerPixel (%d) ...\n", pixelbyte);
		return -1;
	}

	draw_fn = put_pixel_fns[3 + pixelbyte-2];

	if (alpha)
		draw_fn = alpha_pixel_fns[pixelbyte-2];

	pr_debug("%s: %s to %d,%d scale %d,%d txt 0x%x, back 0x%x, alpha %d\n",
		__func__, string, sx, sy, hscale, vscale,
		text_color, back_color, alpha);

	text_draw_eng0816(string, text_color, back_color,
				sx, sy, hscale, vscale,
				framebase, xresol, yresol,
				draw_fn);

	return 0;
}
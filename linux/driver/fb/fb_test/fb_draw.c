#include <stdio.h>
#include <stdlib.h>		/* malloc */
#include <string.h> 	/* strerror */
#include <errno.h> 		/* errno */
#include <fcntl.h> 		/* for O_RDWR */

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
typedef unsigned int 		u32;
#endif	/* u32 */
#ifndef u16
typedef unsigned short 		u16;
#endif	/* u16 */
#ifndef u8
typedef unsigned char 		u8;
#endif	/* u8  */

//#ifndef BITMAPFILEHEADER
typedef struct tagBITMAPFILEHEADER {
// 	u16 	bfType;
  	u32   	bfSize;
  	u16 	bfReserved1;
	u16 	bfReserved2;
  	u32 	bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;
//#endif

//#ifndef BITMAPINFOHEADER
typedef struct tagBITMAPINFOHEADER {
  	u32 		biSize;
  	u32 		biWidth;
  	u32 		biHeight;
  	u16 		biPlanes;
  	u16			biBitCount;
  	u32 		biCompression;
  	u32 		biSizeImage;
  	u32 		biXPelsPerMeter;
  	u32 		biYPelsPerMeter;
  	u32 		biClrUsed;
  	u32 		biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;
//#endif

//#ifndef RGBQUAD
typedef struct tagRGBQUAD {
	u8 		rgbBlue;
	u8 		rgbGreen;
	u8 		rgbRed;
	u8 		rgbReserved;
} RGBQUAD, *PRGBQUAD;
//#endif

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

#define	RGB888TO565(col) 	((((col>>16)&0xFF)&0xF8)<<8) | ((((col>>8)&0xFF)&0xFC)<<3) | ((((col>>0 )&0xFF)&0xF8)>>3)
#define	RGB555TO565(col) 	(((col>>10)&0x1F) << 11) | (((col>> 5)&0x1F) << 6) | ((col<< 0)&0x1F)

void
PutPixel888To565(unsigned long base,
			int xpos, int ypos, int width, int height, u32 color)
{
	*(u16*)(base + (ypos * width + xpos) * 2) = (u16)RGB888TO565(color);
}

void
PutPixel555To565(unsigned long base,
			int xpos, int ypos, int width, int height, u32 color)
{
	*(u16*)(base + (ypos * width + xpos) * 2) = (u16)RGB555TO565(color);
}

void
PutPixel565To565(unsigned long base,
			int xpos, int ypos, int width, int height, u32 color)
{
	*(u16*)(base + (ypos * width + xpos) * 2) = (u16)color & 0xFFFF;
}

void
PutPixel888To888(unsigned long base,
			int xpos, int ypos, int width, int height, u32 color)
{
	*(u8*)(base + (ypos * width + xpos) * 3 + 0) = ((color>> 0)&0xFF);	// B
	*(u8*)(base + (ypos * width + xpos) * 3 + 1) = ((color>> 8)&0xFF);	// G
	*(u8*)(base + (ypos * width + xpos) * 3 + 2) = ((color>>16)&0xFF);	// R
}

void
PutPixel565To888(unsigned long base,
			int xpos, int ypos, int width, int height, u32 color)
{
	*(u8*)(base + (ypos * width + xpos) * 3 + 0) = (((color >> 0 ) << 3) & 0xf8) | (((color >> 0 ) >> 2) & 0x7);	// B
	*(u8*)(base + (ypos * width + xpos) * 3 + 1) = (((color >> 5 ) << 2) & 0xfc) | (((color >> 5 ) >> 4) & 0x3);	// G
	*(u8*)(base + (ypos * width + xpos) * 3 + 2) = (((color >> 11) << 3) & 0xf8) | (((color >> 11) >> 2) & 0x7);	// R
}

void
PutPixel888To8888(unsigned long base,
			int xpos, int ypos, int width, int height, u32 color)
{
	*(u32*)(base + (ypos * width + xpos) * 4) = (u32)(color | (0xFF<<24));
}

void
PutPixel565To8888(unsigned long base,
			int xpos, int ypos, int width, int height, u32 color)
{
	*(u8*)(base + (ypos * width + xpos) * 4 + 0) = (((color >> 0 ) << 3) & 0xf8) | (((color >> 0 ) >> 2) & 0x7);	// B
	*(u8*)(base + (ypos * width + xpos) * 4 + 1) = (((color >> 5 ) << 2) & 0xfc) | (((color >> 5 ) >> 4) & 0x3);	// G
	*(u8*)(base + (ypos * width + xpos) * 4 + 2) = (((color >> 11) << 3) & 0xf8) | (((color >> 11) >> 2) & 0x7);	// R
	*(u8*)(base + (ypos * width + xpos) * 4 + 3) = 0xFF;	// No Alpha
}

void (*PUTPIXELTABLE[])(unsigned long, int, int, int, int, u32) =
{
	PutPixel555To565,
	PutPixel565To888,
	PutPixel565To8888,
	PutPixel888To565,
	PutPixel888To888,
	PutPixel888To8888,
};

static inline void	PutPixel_UYVY(unsigned long FB, u32 X, u32 Y, u8 *pY, u8 *pU, u8 *pV, u32 Stride)
{
	/* UY0VY1 */
	*(u8 *)(FB + ((Y+0) * Stride) + X + 0) = (u8)((pU[0] + pU[1]) >> 1);
	*(u8 *)(FB + ((Y+0) * Stride) + X + 1) = (u8)(pY[0]);
	*(u8 *)(FB + ((Y+0) * Stride) + X + 2) = (u8)((pV[0] + pV[1]) >> 1);
	*(u8 *)(FB + ((Y+0) * Stride) + X + 3) = (u8)(pY[1]);

	/* UY0VY1 */
	*(u8 *)(FB + ((Y+1) * Stride) + X + 0) = (u8)((pU[2] + pU[3]) >> 1);
	*(u8 *)(FB + ((Y+1) * Stride) + X + 1) = (u8)(pY[2]);
	*(u8 *)(FB + ((Y+1) * Stride) + X + 2) = (u8)((pV[2] + pV[3]) >> 1);
	*(u8 *)(FB + ((Y+1) * Stride) + X + 3) = (u8)(pY[3]);
}

static void	RGB2YUV(u8 R, u8 G, u8 B, u8 *pY, u8 *pU, u8 *pV)
{
#if 1
#if 1
	// Y, C = 0 ~ 255
	*pY = (u8)(((  66 * (int)R + 129 * (int)G +  25 * (int)B + 128) >> 8) +  16);
	*pU = (u8)((( -38 * (int)R -  74 * (int)G + 112 * (int)B + 128) >> 8) + 128);
	*pV = (u8)((( 112 * (int)R -  94 * (int)G -  18 * (int)B + 128) >> 8) + 128);
#else
	*pY = (u8)(( 0.299*R) + (0.587*G) + (0.114*B)) + 16;
	*pU = (u8)((-0.148*R) - (0.291*G) + (0.439*B)) + 128;
	*pV = (u8)(( 0.439*R) - (0.368*G) - (0.071*B)) + 128;
#endif
#else
	// Y = 16 ~ 235
	// C = 16 ~ 240
	*pY = (u8)( 0.299*R + 0.587*G + 0.114*B);
	*pU = (u8)(-0.147*R - 0.289*G + 0.436*B) + 128;	// cbcr range = -128~127, spica support up to 0, so add 128
	*pV = (u8)( 0.615*R - 0.515*G - 0.100*B) + 128;	// cbcr range = -128~127, spica support up to 0, so add 128
#endif
}

//------------------------------------------------------------------------------
void * load_bmp2rgb (const char *name,
					unsigned long framebase, int xresol, int yresol,
					int pixelbyte)
{
	u16				  BMP_ID;
	BITMAPFILEHEADER  BMPFile;
	BITMAPINFOHEADER  BMPInfo;

	u8   * pBitMap  = NULL;
	int BMPPixelByte;

	int lcdsx, lcdsy, lcdex, lcdey;
	int bmpsx, bmpsy, bmpex, bmpey;
	int lx, ly, bx, by;

	u8 *pPixel;
	u32 color;
	u32 BMP_Align;

	struct stat fsta;

	int    fd;
	char   path[256] = { 0, };
	u8  *  buffer;
	unsigned long  length;
	int ret;

	void (*putpixel)(unsigned long, int, int, int, int, u32) = NULL;

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
		return;
	}

	// Get BMP header
	ret = read(fd, (void*)&BMPFile, sizeof(BITMAPFILEHEADER));
	ret = read(fd, (void*)&BMPInfo, sizeof(BITMAPINFOHEADER));

#if (DEBUG_BMPHEADER == 1)
	printf("\nBMP File Heade \r\n");
	printf("Type	: 0x%x \r\n", BMP_ID);
	printf("Size	: %d   \r\n", BMPFile.bfSize);
	printf("Offs	: %d   \r\n", BMPFile.bfOffBits);
	printf("\nBMP Info Header  \r\n");
	printf("Size     : %d\r\n", BMPInfo.biSize);
	printf("width    : %d\r\n", BMPInfo.biWidth);
	printf("height   : %d\r\n", BMPInfo.biHeight);
	printf("Planes   : %d\r\n", BMPInfo.biPlanes);
	printf("BitCount : %d\r\n", BMPInfo.biBitCount);
	printf("Compress : %d\r\n", BMPInfo.biCompression);
	printf("SizeImage: %d\r\n", BMPInfo.biSizeImage);
	printf("XPels    : %d\r\n", BMPInfo.biXPelsPerMeter);
	printf("YPels    : %d\r\n", BMPInfo.biYPelsPerMeter);
	printf("ClrUsed  : %d\r\n", BMPInfo.biClrUsed);
	printf("ClrImport: %d\r\n", BMPInfo.biClrImportant);
	printf("\r\n");
#endif
	BMPPixelByte = BMPInfo.biBitCount/8;

	/* a line size of BMP must be aligned on DWORD boundary. */
	length = ((((BMPInfo.biWidth * 3) + 3) / 4) * 4) * BMPInfo.biHeight;

	/* Load bmp to memory */
	buffer = (u8 *)malloc(length);
	if (! buffer) {
		printf("error, allocate memory (size: %ld)...\n", length);
		close(fd);
		return 0;
	}

	lseek(fd, (int)(BMPFile.bfOffBits), SEEK_SET);	// BMP file end point.
	ret = read(fd, buffer, length);
	close(fd);

	/* set bmp base */
	pBitMap = (u8*)(buffer);
	lcdsx   = 0, lcdsy = 0, lcdex = xresol, lcdey = yresol;
	bmpsx   = 0, bmpsy = 0, bmpex = BMPInfo.biWidth-1, bmpey = BMPInfo.biHeight-1;

	printf("DONE: BMP %d by %d (%dbpp), len=%d \r\n",
		BMPInfo.biWidth, BMPInfo.biHeight, BMPPixelByte, BMPFile.bfSize);
	printf("DRAW: on 0x%08lx \r\n", framebase);

	if ((int)BMPInfo.biWidth  > xresol)
	{
		bmpsx = (BMPInfo.biWidth - xresol)/2;
		bmpex = bmpsx + xresol;
	}
	else if ((int)BMPInfo.biWidth  < xresol)
	{
		lcdsx += (xresol- BMPInfo.biWidth)/2;
		lcdex  = lcdsx + BMPInfo.biWidth;
	}

	if ((int)BMPInfo.biHeight > yresol)
	{
		bmpsy = (BMPInfo.biHeight - yresol)/2;
		bmpey = bmpsy + yresol;
	}
	else if ((int)BMPInfo.biHeight < yresol)
	{
		lcdsy += (yresol- BMPInfo.biHeight)/2;
		lcdey  = lcdsy + BMPInfo.biHeight;
	}

	// Select put pixel function
	//
	switch(BMPPixelByte)
	{
	case 2:	putpixel = PUTPIXELTABLE[0 + pixelbyte-2];	break;	// 565 To 565/888
	case 3: putpixel = PUTPIXELTABLE[3 + pixelbyte-2];	break;	// 888 To 565/888
	default:
		pr_debug("\nNot support BitPerPixel (%d) ...\r\n", BMPPixelByte);
		return NULL;
	}

	/*
	 * BMP stride ,
	 * a line size of BMP must be aligned on DWORD boundary.
	 */
	BMP_Align = (((BMPInfo.biWidth*3)+3)/4)*4;

	// Draw 16 BitperPixel image on the frame buffer base.
	// RGB555
	if (BMPPixelByte == 2 && BMPInfo.biCompression == 0x00000000)
	{
		for(ly = lcdsy, by = bmpey; by>=bmpsy; ly++, by--)
		{
			for(lx = lcdsx, bx = bmpsx; bx<=bmpex; lx++, bx++)
			{
				color = *(u16*)(pBitMap + (by * BMP_Align) + (bx * BMPPixelByte));
				color = (u16)RGB555TO565(color);
				putpixel(framebase, lx, ly, xresol, yresol, color);
			}
		}
	}

	// Draw 16 BitperPixel image on the frame buffer base.
	// RGB565
	if (BMPPixelByte == 2 && BMPInfo.biCompression == 0x00000003)
	{
		for(ly = lcdsy, by = bmpey; by>=bmpsy; ly++, by--)
		{
			for(lx = lcdsx, bx = bmpsx; bx<=bmpex; lx++, bx++)
			{
				color = *(u16*)(pBitMap + (by * BMP_Align) + (bx * BMPPixelByte));
				putpixel(framebase, lx, ly, xresol, yresol, color);
			}
		}
	}

	// Draw 24 BitperPixel image on the frame buffer base.
	//
	if (BMPPixelByte == 3)
	{
		u32	RColor, GColor, BColor;
		for(ly = lcdsy, by = bmpey; by>=bmpsy; ly++, by--)
		{
			for(lx = lcdsx, bx = bmpsx; bx<=bmpex; lx++, bx++)
			{
				pPixel = (u8*)(pBitMap + (by * BMP_Align) + (bx * BMPPixelByte));
				RColor  = *(pPixel+2);
				GColor  = *(pPixel+1);
				BColor  = *(pPixel+0);
				if (2 == pixelbyte) {
					RColor  = dither_pattern5[RColor & 0x7][lx%3][ly%3] + (RColor>>3);	RColor= (RColor>31) ? 31: RColor;
					GColor  = dither_pattern6[GColor & 0x3][lx%2][ly%2] + (GColor>>2);	GColor= (GColor>63) ? 63: GColor;
					BColor  = dither_pattern5[BColor & 0x7][lx%3][ly%3] + (BColor>>3);	BColor= (BColor>31) ? 31: BColor;
					color	= (RColor<<11) | (GColor<<5) | (BColor);
					*(u16*)(framebase + (ly * xresol + lx) * 2) = (u16)color;
				} else	 {
					color = ((RColor&0xFF)<<16) | ((GColor&0xFF)<<8) | (BColor&0xFF);
					putpixel(framebase, lx, ly, xresol, yresol, color);
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
	u16				  BMP_ID;
	BITMAPFILEHEADER  BMPFile;
	BITMAPINFOHEADER  BMPInfo;

	u8   * pBitMap  = NULL;
	int BMPPixelByte;

	int lcdsx, lcdsy, lcdex, lcdey;
	int bmpsx, bmpsy, bmpex, bmpey;
	int lx, ly, bx, by;

	u8 *pPixel;
	u32 color;

	struct stat fsta;

	int    fd;
	char   path[256] = { 0, };
	u8  *  buffer;
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
		printf("Fail: %s Unknonw BMP (type:0x%x, len=%d) ...\n", path, BMP_ID, (int)fsta.st_size);
		return;
	}

	// Get BMP header
	ret = read(fd, (void*)&BMPFile, sizeof(BITMAPFILEHEADER));
	ret = read(fd, (void*)&BMPInfo, sizeof(BITMAPINFOHEADER));

#if (DEBUG_BMPHEADER == 1)
	printf("\nBMP File Heade \r\n");
	printf("Type	: 0x%x \r\n", BMP_ID);
	printf("Size	: %d   \r\n", BMPFile.bfSize);
	printf("Offs	: %d   \r\n", BMPFile.bfOffBits);
	printf("\nBMP Info Header  \r\n");
	printf("Size     : %d\r\n", BMPInfo.biSize);
	printf("width    : %d\r\n", BMPInfo.biWidth);
	printf("height   : %d\r\n", BMPInfo.biHeight);
	printf("Planes   : %d\r\n", BMPInfo.biPlanes);
	printf("BitCount : %d\r\n", BMPInfo.biBitCount);
	printf("Compress : %d\r\n", BMPInfo.biCompression);
	printf("SizeImage: %d\r\n", BMPInfo.biSizeImage);
	printf("XPels    : %d\r\n", BMPInfo.biXPelsPerMeter);
	printf("YPels    : %d\r\n", BMPInfo.biYPelsPerMeter);
	printf("ClrUsed  : %d\r\n", BMPInfo.biClrUsed);
	printf("ClrImport: %d\r\n", BMPInfo.biClrImportant);
	printf("\r\n");
#endif
	BMPPixelByte = BMPInfo.biBitCount/8;

	/* a line size of BMP must be aligned on DWORD boundary. */
	length = ((((BMPInfo.biWidth * 3) + 3) / 4) * 4) * BMPInfo.biHeight;

	/* Load bmp to memory */
	buffer = (u8 *)malloc(length);
	if (! buffer) {
		printf("error, allocate memory (size: %ld)...\n", length);
		close(fd);
		return 0;
	}

	lseek(fd, (int)(BMPFile.bfOffBits), SEEK_SET);	// BMP file end point.
	ret = read(fd, buffer, length);
	close(fd);

	/* set bmp base */
	pBitMap = (u8*)(buffer);
	lcdsx   = 0, lcdsy = 0, lcdex = xresol, lcdey = yresol;
	bmpsx   = 0, bmpsy = 0, bmpex = BMPInfo.biWidth-1, bmpey = BMPInfo.biHeight-1;

	printf("DONE: BMP %d by %d (%dbpp), len=%d \r\n",
		BMPInfo.biWidth, BMPInfo.biHeight, BMPPixelByte, BMPFile.bfSize);
	printf("DRAW: on 0x%08lx \r\n", framebase);

	if ((int)BMPInfo.biWidth  > xresol)
	{
		bmpsx = (BMPInfo.biWidth - xresol)/2;
		bmpex = bmpsx + xresol;
	}
	else if ((int)BMPInfo.biWidth  < xresol)
	{
		lcdsx += (xresol- BMPInfo.biWidth)/2;
		lcdex  = lcdsx + BMPInfo.biWidth;
		lcdsx  = ((lcdsx + 3)/4) *4;	// align 4 pixel for YUYV
	}

	if ((int)BMPInfo.biHeight > yresol)
	{
		bmpsy = (BMPInfo.biHeight - yresol)/2;
		bmpey = bmpsy + yresol;
	}
	else if ((int)BMPInfo.biHeight < yresol)
	{
		lcdsy += (yresol- BMPInfo.biHeight)/2;
		lcdey  = lcdsy + BMPInfo.biHeight;
	}

	// Draw 16 BitperPixel image on the frame buffer base.
	// RGB555
	if (BMPPixelByte == 2 && BMPInfo.biCompression == 0x00000000)
	{
		printf("Fail: %s not support %d Bpp\n", path, BMPPixelByte*8);
	}

	// Draw 16 BitperPixel image on the frame buffer base.
	// RGB565
	if (BMPPixelByte == 2 && BMPInfo.biCompression == 0x00000003)
	{
		printf("Fail: %s not support %d Bpp\n", path, BMPPixelByte*8);
	}

	// Draw 24 BitperPixel image on the UYVY frame buffer
	if (BMPPixelByte == 3)
	{
		u8	R, G, B, Y[2][2], U[2][2], V[2][2];
		u32 LCD_Align = xresol * pixelbyte;			/* LCD stride */
		u32 BMP_Align = (((BMPInfo.biWidth*3)+3)/4)*4;	/* BMP stride ,
														  a line size of BMP must be aligned on DWORD boundary. */

	//	printf("lcd sx=%4d, ex=%4d, sy=%4d, ey=%4d, stride=%6d\n", lcdsx, lcdex, lcdsy, lcdey, LCD_Align);
	//	printf("bmp sx=%4d, ex=%4d, sy=%4d, ey=%4d, stride=%6d\n", bmpsx, bmpex, bmpsy, bmpey, BMP_Align);
		for(ly = lcdsy, by = bmpey; by > bmpsy; ly+=2, by-=2)
		{
			for(lx = lcdsx*pixelbyte, bx = bmpsx; bmpex >= bx; lx+=(pixelbyte + 2), bx+=2)
			{
				pPixel = (u8*)(pBitMap + (by * BMP_Align) + (bx * BMPPixelByte));

				R  = *(pPixel+2);
				G  = *(pPixel+1);
				B  = *(pPixel+0);
				RGB2YUV((int)R, (int)G, (int)B, &Y[0][0], &U[0][0], &V[0][0] );

				R  = *(pPixel+5);
				G  = *(pPixel+4);
				B  = *(pPixel+3);
				RGB2YUV((int)R, (int)G, (int)B, &Y[0][1], &U[0][1], &V[0][1] );

				R  = *(pPixel+2 - BMP_Align);
				G  = *(pPixel+1 - BMP_Align);
				B  = *(pPixel+0 - BMP_Align);
				RGB2YUV((int)R, (int)G, (int)B, &Y[1][0], &U[1][0], &V[1][0] );

				R  = *(pPixel+5 - BMP_Align);
				G  = *(pPixel+4 - BMP_Align);
				B  = *(pPixel+3 - BMP_Align);
				RGB2YUV((int)R, (int)G, (int)B, &Y[1][1], &U[1][1], &V[1][1] );
			#if (0)
				Y[0][1] = Y[0][0], U[0][1] = U[0][0], V[0][1] = V[0][0];
				Y[1][1] = Y[1][0], U[1][1] = U[1][0], V[1][1] = V[1][0];
			#endif

				PutPixel_UYVY(framebase, (u32)lx, (u32)ly , &Y[0][0], &U[0][0], &V[0][0], LCD_Align);
			}
		//	printf("[%4d:%4d, %d:%d, %d]-----------------------------\n", ly, lx, by, bx, BMP_Align);
		}
	}
	return (void*)buffer;
}

void   unload_bmp(void *buffer)
{
	if (buffer)
		free(buffer);
}

/*-----------------------------------------------------------------------------*/

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
	int Stride = xresol * pixelbyte;
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
		PutPixel_UYVY(framebase, (u32)x, (u32)y , &Y[0][0], &U[0][0], &V[0][0], Stride);
	//	printf("%d:Y %3d,U %3d,V %3d\n", color, pYUV[0], pYUV[1], pYUV[2]);
	}
	}
}

void fill_rgb (unsigned long framebase,
					int xresol, int yresol, int pixelbyte,
					int startx, int starty, int width, int height,
					unsigned int color)
{
	int x, y, sx, sy, ex, ey;
	void (*putpixel)(unsigned long, int, int, int, int, u32) = NULL;

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

	// Select put pixel function
	putpixel = PUTPIXELTABLE[3 + pixelbyte-2];

	for(y = sy; ey > y; y++)
	for(x = sx; ex > x; x++)
		putpixel(framebase, x, y, xresol, yresol, color);
}

void copy_uyuv(unsigned long framebase,
					int xresol, int yresol, int pixelbyte,
					int startx, int starty, int image_width, int image_height,
					int image_pixel, unsigned long image_base)
{
	u8 *pSrc, *pDst;
	int Stride = xresol * pixelbyte;
	int img_w, img_h;;
	int lcd_w, lcd_h;
	int x, y;

	lcd_w = xresol - startx;
	lcd_h = yresol - starty;

	if (lcd_w > image_width)
		img_w = image_width * image_pixel;
	else
		img_w = lcd_w * image_pixel;

	if (lcd_h > image_height)
		img_h = image_height;
	else
		img_h = lcd_h;

	// align 4byte for YUYV
	startx &= ~((1<<2) - 1);

//	printf("H=%d, W=%d\n", img_h, img_w);
	for(y = 0; img_h > y; y++) {
		pDst = (u8*)(framebase + (startx * pixelbyte) + ((y + starty) * Stride));
		pSrc = (u8*)(image_base + (y * image_width * image_pixel));
		memcpy(pDst, pSrc, img_w);
//		printf("[%4d][%d, %d]\n", y,
//			(startx * pixelbyte) + ((y + starty) * Stride),
//			(y * image_width * image_pixel));
	}
}

void copy_rgb (unsigned long framebase,
					int xresol, int yresol, int pixelbyte,
					int startx, int starty, int image_width, int image_height,
					int image_pixel, unsigned long image_base)
{
	u8 *pSrc, *pDst;
	int Stride = xresol * pixelbyte;
	int img_w, img_h;;
	int lcd_w, lcd_h;
	int x, y;

	lcd_w = xresol - startx;
	lcd_h = yresol - starty;

	if (lcd_w > image_width)
		img_w = image_width * image_pixel;
	else
		img_w = lcd_w * image_pixel;

	if (lcd_h > image_height)
		img_h = image_height;
	else
		img_h = lcd_h;

	pr_debug("%s: h=%d, w=%d\n", __func__, img_h, img_w);
	for(y = 0; img_h > y; y++) {
		pDst = (u8*)(framebase + (startx * pixelbyte) + ((y + starty) * Stride));
		pSrc = (u8*)(image_base + (y * image_width * image_pixel));
		memcpy(pDst, pSrc, img_w);
	//	printf("[%4d][%d, %d]\n", y,
	//		(startx * pixelbyte) + ((y + starty) * Stride),
	//		(y * image_width * image_pixel));
	}
}

int get_bmp_info(const char *name, int *width, int *height, int *bitcount)
{
	u16					BMP_ID;
	BITMAPFILEHEADER 	BMPFile;
	BITMAPINFOHEADER 	BMPInfo;

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

	ret = read(fd, (void*)&BMPFile, sizeof(BITMAPFILEHEADER));
	ret = read(fd, (void*)&BMPInfo, sizeof(BITMAPINFOHEADER));

	bmp_w   = (int)BMPInfo.biWidth;
	bmp_h   = (int)BMPInfo.biHeight;
	bmp_bit = (int)BMPInfo.biBitCount;
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
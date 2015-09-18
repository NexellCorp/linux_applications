#include <stdio.h>
#include <unistd.h> 	/* read */
#include <fcntl.h> 		/* for O_RDWR */
#include <string.h> 	/* strerror */
#include <errno.h> 		/* errno */

#include <sys/mman.h>	/* for mmap options */
#include <sys/ioctl.h>	/* for ioctl */

#include <linux/fb.h>	/* frame buffer */

#include "fb_dev.h"

#define NXPFB_SET_POS	 	_IOW('N', 104, __u32)

/*-----------------------------------------------------------------------------
 * Frame buffer device APIs
 -----------------------------------------------------------------------------*/
int fb_open(const char *name)
{
	int fd = open(name, O_RDWR);
	if (0 > fd)	{
		printf("Fail: %s open: %s\n", name, strerror(errno));
		return -1;
	}
	return fd;
}

void fb_close(int fd)
{
	if (fd >= 0)
		close(fd);
}

int fb_mmap(int fd, unsigned long *base, unsigned long *len)
{
	void * fb_base = NULL;
	struct fb_var_screeninfo var;
	unsigned long  fb_len;
	__assert__(fd >= 0);

	if (0 > ioctl(fd, FBIOGET_VSCREENINFO, &var)) {
		printf("Fail: ioctl(0x%x): %s\n", FBIOGET_VSCREENINFO, strerror(errno));
		return -1;
	}

	fb_len  = (var.xres * var.yres_virtual * var.bits_per_pixel/8);
	fb_base = (void*)mmap(0, fb_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if ((unsigned long)fb_base == (unsigned long)(-1)) {
		printf("Fail: mmap: %s\n", strerror(errno));
		return -1;
	}

	if (base)
		*base = (unsigned long)fb_base;

	if (len)
		*len = fb_len;

	return 0;
}

void fb_munmap(unsigned long base, unsigned long len)
{
	if (base && len)
		munmap((void*)base, len);
}

int fb_get_resol(int fd, int *width, int *height, int *pixbyte, int *buffcnt)
{
	struct fb_var_screeninfo var;
	int ret;
	__assert__(fd >= 0);

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: ioctl(0x%x): %s\n", FBIOGET_VSCREENINFO, strerror(errno));
		return ret;
	}

	if (width)
		*width  = var.xres;

	if (height)
		*height = var.yres;

	if (pixbyte)
		*pixbyte = var.bits_per_pixel/8;

	if (buffcnt)
		*buffcnt = var.yres_virtual/var.yres;

	return 0;
}

int fb_set_resol(int fd, int startx, int starty, int width, int height, int pixbyte)
{
	struct fb_var_screeninfo var;
	int pos[3] = { 0, };	/* left, right, waitsycn */
	int ret = 0;
	__assert__(fd >= 0);

	/*  1. Get var */
	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: ioctl(0x%x): %s\n", FBIOGET_VSCREENINFO, strerror(errno));
		return ret;
	}

	var.xres = width;
	var.yres = height;
	var.bits_per_pixel = pixbyte*8;

	/*  2. Set var */
	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: ioctl(0x%x): %s\n", FBIOPUT_VSCREENINFO, strerror(errno));
		return ret;
	}

	if (0 == startx && 0 == starty)
		return 0;

	pos[0] = startx;
	pos[1] = starty;
	pos[2] = 1;

	ret = ioctl(fd, NXPFB_SET_POS, pos);
	if (0 > ret) {
		printf("Fail: ioctl(0x%x): %s\n", NXPFB_SET_POS, strerror(errno));
		return ret;
	}

	return 0;
}

int  fb_pos(int fd, int startx, int starty)
{
	int pos[3] = { 0, };	/* left, right, waitsycn */
	int ret = 0;
	__assert__(fd >= 0);

	if (0 == startx && 0 == starty)
		return 0;

	pos[0] = startx;
	pos[1] = starty;
	pos[2] = 1;

	ret = ioctl(fd, NXPFB_SET_POS, pos);
	if (0 > ret) {
		printf("Fail: ioctl(0x%x): %s\n", NXPFB_SET_POS, strerror(errno));
		return ret;
	}

	return 0;
}

void fb_flip(int fd, int buffno)
{
	struct fb_var_screeninfo var;
	int ret;
	__assert__(fd >= 0);

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: ioctl(0x%x): %s\n", FBIOGET_VSCREENINFO, strerror(errno));
		return;
	}

	if (buffno > (int)(var.yres_virtual/var.yres))
		return;

	/* set buffer */
	var.yoffset = (var.yres * buffno);

	ret = ioctl(fd, FBIOPAN_DISPLAY, &var);
	if (0 > ret)
		printf("Fail: ioctl(0x%x): %s\n", FBIOPAN_DISPLAY, strerror(errno));
}

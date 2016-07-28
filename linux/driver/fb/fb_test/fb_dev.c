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

/*
 * Frame buffer device APIs
 */
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

int fb_mmap(int fd, unsigned long *vaddr,
			unsigned long *paddr, unsigned long *len)
{
	void * fb_base = NULL;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	unsigned long  fb_len;
	__assert__(fd >= 0);

	if (0 > ioctl(fd, FBIOGET_FSCREENINFO, &fix)) {
		printf("Fail: FBIOGET_FSCREENINFO %s\n", strerror(errno));
		return -1;
	}

	if (0 > ioctl(fd, FBIOGET_VSCREENINFO, &var)) {
		printf("Fail: FBIOGET_VSCREENINFO %s\n", strerror(errno));
		return -1;
	}

	fb_len  = (var.xres_virtual * var.yres_virtual * var.bits_per_pixel/8);
	fb_base = (void*)mmap(0, fb_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if ((unsigned long)fb_base == (unsigned long)(-1)) {
		printf("Fail: mmap %s\n", strerror(errno));
		return -1;
	}

	if (paddr) *paddr = (unsigned long)fix.smem_start;
	if (vaddr) *vaddr = (unsigned long)fb_base;

	if (len)
		*len = fb_len;

	return 0;
}

void fb_munmap(unsigned long vaddr, unsigned long len)
{
	if (vaddr && len)
		munmap((void*)vaddr, len);
}

int fb_get_resol(int fd, int *width, int *height,
			int *pixbyte, int *buffcnt, int *x_pitch, int *y_pitch)
{
	struct fb_var_screeninfo var;
	int ret;
	__assert__(fd >= 0);

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: FBIOGET_VSCREENINFO %s\n", strerror(errno));
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

	if (x_pitch)
		*x_pitch  = var.xres_virtual;

	if (y_pitch)
		*y_pitch = var.yres_virtual;

	return 0;
}

void fb_flip(int fd, int nr)
{
	struct fb_var_screeninfo var;
	int avail;
	int ret;

	__assert__(fd >= 0);

	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: FBIOGET_VSCREENINFO %s\n", strerror(errno));
		return;
	}

	avail = var.yres_virtual/var.yres;
	if (nr > avail) {
		printf("Fail: fb buffer %d is over support buffers %d\n", nr, avail);
		return;
	}

	/* set buffer */
	var.yoffset = (var.yres * nr);

	ret = ioctl(fd, FBIOPAN_DISPLAY, &var);
	if (0 > ret)
		printf("Fail: FBIOPAN_DISPLAY %s\n", strerror(errno));
}

int fb_set_resol(int fd,
			int startx, int starty, int width, int height, int pixbyte)
{
	struct fb_var_screeninfo var;
	int pos[3] = { 0, };	/* left, right, waitsycn */
	int ret = 0;
	__assert__(fd >= 0);

	/*  1. Get var */
	ret = ioctl(fd, FBIOGET_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: FBIOGET_VSCREENINFO %s\n", strerror(errno));
		return ret;
	}

	var.xres = width;
	var.yres = height;
	var.bits_per_pixel = pixbyte*8;

	/*  2. Set var */
	ret = ioctl(fd, FBIOPUT_VSCREENINFO, &var);
	if (0 > ret) {
		printf("Fail: FBIOPUT_VSCREENINFO %s\n", strerror(errno));
		return ret;
	}

	if (0 == startx && 0 == starty)
		return 0;

	pos[0] = startx;
	pos[1] = starty;
	pos[2] = 1;

	ret = ioctl(fd, NXPFB_SET_POS, pos);
	if (0 > ret) {
		printf("Fail: NXPFB_SET_POS %s\n", strerror(errno));
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
		printf("Fail: NXPFB_SET_POS %s\n", strerror(errno));
		return ret;
	}

	return 0;
}



#include <stdio.h>
#include <unistd.h> 	/* read */
#include <fcntl.h> 		/* for O_RDWR */
#include <stdlib.h>		/* malloc */
#include <string.h> 	/* strerror */
#include <errno.h> 		/* errno */
#include <pthread.h>

#include "fb_dev.h"
#include "fb_draw.h"

#define FB_DEF_FORMAT	FOURCC_XRGB		/* FOURCC_XRGB, FOURCC_UYVY */
#define FILL_COLOR		0x0000FF

#define	FB_DEV_PATH		"/dev/fb0"

typedef struct {
	int	fd;
	int	xresol;
	int	yresol;
	int	pixelbyte;
	unsigned long base;
	unsigned long length;
	int buffers;
} fb_param;

static fb_param *fb_init_par(char *path)
{
	fb_param *fpar;
	unsigned long base = 0, len = 0;
	int w = 0, h = 0, pixel = 0, bn = 1;
	int fd;

	fd = fb_open(path);
	if (0 > fd)
		return NULL;

	fpar = (fb_param*)malloc(sizeof(fb_param));
	if (!fpar) {
		printf("failed malloc size %ld, %s\n", sizeof(fb_param), strerror(errno));
		return NULL;
	}
	memset((void*)fpar, 0, sizeof(fb_param));

	if (0 > fb_mmap(fd, &base, &len))
		goto err_fb;

	if (0 > fb_get_resol(fd, &w, &h, &pixel, &bn))
		goto err_fb;

	fpar->fd = fd;
	fpar->base = base;
	fpar->length = len;
	fpar->xresol = w;
	fpar->yresol = h;
	fpar->pixelbyte = pixel;
	fpar->buffers = bn;

	printf("%s: virt = 0x%08lx, %8ld (%4d * %4d, %d bpp, %d buffers)\n",
		path, base, len, w, h, pixel, bn);

	return fpar;

err_fb:
	if (base)
		fb_munmap(base, len);

	if (fd >= 0)
		fb_close(fd);

	if (fpar)
		free(fpar);

	return NULL;
}

static void fb_exit_par(fb_param *fpar)
{
	unsigned long base, length;
	int fd;
	__assert__(fpar);

	fd = fpar->fd;
	base = fpar->base;
	length = fpar->length;

	if (0 > fd)
		goto exit_out;

	fb_flip(fd, 0);	/* restore fpar */

	if (base)
		fb_munmap(base, length);

	fb_close(fd);

exit_out:
	if (fpar)
		free(fpar);
}

static int fb_change_resol(char *path, int sx, int sy, int width, int height, int pixelbyte)
{
	int fd, ret;

	fd = fb_open(path);
	if (0 > fd)
		return -EINVAL;

	ret = fb_set_resol(fd, sx, width, sy, height, pixelbyte);

	fb_close(fd);

	return ret;
}

static int fb_change_pos(char *path, int sx, int sy)
{
	int fd, ret;

	fd = fb_open(path);
	if (0 > fd)
		return -EINVAL;

	ret = fb_pos(fd, sx, sy);

	fb_close(fd);

	return ret;
}

static void fb_fill_rect(fb_param *fpar, int sx, int sy, int w, int h,
					unsigned int color, int random)
{
	unsigned long base;
	int xres, yres, pixel;
	int x1, y1, x2, y2;

	if (sx > fpar->xresol || sy > fpar->yresol ||
		w  > fpar->xresol || h  > fpar->yresol ) {
		printf("over sx %d, sy %d, w %d, h %d (resol %d x %d) \n",
			sx, sy, w, h, fpar->xresol, fpar->yresol);
		return;
	}

	base = fpar->base;
	xres = fpar->xresol;
	yres = fpar->yresol;
	pixel = fpar->pixelbyte;

	printf("fill: %d~%d, %d~%d (%dx%d, %dpix) 0x%08x [%s]\n",
		sx, w, sy, h, xres, yres, pixel, color, random?"random":"fill");

	if (!random) {
		fill_rgb(base, xres, yres, pixel, sx, sy, w, h, color);
		return;
	}

	srand(1);

	while (1) {
		/* random number between 0 and xres */
		x1 = (int)(rand() % w);	// left
		x2 = (int)(rand() % (w-x1));	// width

		/* random number between 0 and yres */
		y1 = (int)(rand() % h);	// top
		y2 = (int)(rand() % (h-y1));	// height

		/* swap */
		if (x1 > x2) {
			x1 = x1 + x2, x2 = x1 - x2, x1 = x1 - x2;
		}

		/* swap */
		if (y1 > y2) {
			y1 = y1 + y2, y2 = y1 - y2, y1 = y1 - y2;
		}

		x1 += sx;
		y1 += sy;

		color = (int)rand();

		fill_rgb(base, xres, yres, pixel, x1, y1, x2, y2, color);
	}
}

void print_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("-d framebuffer node path \n");
    printf("-f color  fill framebuffer with input color (hex)\n");
	printf("-r random fill framebuffer \n");
    printf("-p fill framebuffer in position (ex> 0,1024,0,600 : startx,width, starty,height) \n");
	printf("-b load bmp to framebuffer\n");
	printf("-c change resolution (ex> 1024,600,4 : width,height,pixelbyte)\n");
	printf("-m move fb (ex> 0,100 : startx,starty)\n");
}

int main(int argc, char **argv)
{
	int opt;
	fb_param *fpar = NULL;
	char *path = FB_DEV_PATH;
	char *bmp = NULL;
	char *endp;
	unsigned int color = FILL_COLOR;
	int o_random = 0, o_chagne = 0, o_move = 0;
	int sx = 0, sy = 0, w = 0, h = 0, pixelbyte = 0;
	int ret = 0;

    while (-1 != (opt = getopt(argc, argv, "hd:f:p:rb:c:m:"))) {
		switch(opt) {
        case 'h':   print_usage();  exit(0);   	break;
		case 'd':   path = optarg;				break;
		case 'f':   color = strtoul(optarg, NULL, 16);	break;
		case 'r':   o_random = 1;				break;
		case 'p':
					sx = strtoul(optarg, &endp, 10); endp++;
					w = strtoul(endp, &endp, 10); endp++;
					sy = strtoul(endp, &endp, 10); endp++;
					h = strtoul(endp, NULL, 10);
					break;
		case 'c':
					o_chagne = 1;
					w = strtoul(optarg, &endp, 10); endp++;
					h = strtoul(endp, &endp, 10); endp++;
					pixelbyte = strtoul(endp, NULL, 10);
					break;
		case 'm':
					o_move = 1;
					sx = strtoul(optarg, &endp, 10); endp++;
					sy = strtoul(endp, &endp, 10); endp++;
					break;
		case 'b':   bmp = optarg;				break;
		default:   	print_usage(), exit(0);    	break;
		};
	}

	if (o_chagne) {
		ret = fb_change_resol(path, 0, w, 0, h, pixelbyte);
		if (0 > ret)
			return ret;
	}

	if (o_move) {
		ret = fb_change_pos(path, sx, sy);
		if (0 > ret)
			return ret;
	}

	fpar = fb_init_par(path);
	if (!fpar)
		return 0;

	fb_fill_rect(fpar, sx, sy,
		(w ? w : fpar->xresol), (h ? h : fpar->yresol),
		color, o_random);

	fb_exit_par(fpar);

	return 0;
}
#include <stdio.h>
#include <unistd.h> 	/* read */
#include <fcntl.h> 		/* for O_RDWR */
#include <stdlib.h>		/* malloc */
#include <string.h> 	/* strerror */
#include <errno.h> 		/* errno */
#include <sys/signal.h>
#include <sys/time.h>

#include "fb_dev.h"
#include "fb_draw.h"

#define FB_DEF_FORMAT	FOURCC_XRGB		/* FOURCC_XRGB, FOURCC_UYVY */
#define FILL_COLOR		0x0000FF

#define	FB_DEV_PATH		"/dev/fb0"

struct fb_param {
	int	fd;
	int	xresol;
	int	yresol;
	int	pixelbyte;
	unsigned long base;
	unsigned long length;
	int x_pitch;
	int y_pitch;
	int buffers;
} *__fb_parm;

static void signal_handler(int sig)
{
	struct fb_param *fp = __fb_parm;

	printf("\nAborted by signal %s (%d): ",
		(char*)strsignal(sig), sig);

	switch(sig)	{
	case SIGINT:
			/* Interrupt : signal value = 2 */
			printf("SIGINT \n");
			break;
	case SIGTERM:
			/* Terminated : signal value = 15 ( Kill Message ) */
			printf("SIGTERM\n");
			break;
	case SIGSEGV:
			printf("SIGSEGV\n");
			break;
	default: break;
	}

	/* restore fb */
	fb_flip(fp->fd, 0);
	exit(0);
}

static void register_signal(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
}

static struct fb_param *fb_initialize(char *path)
{
	struct fb_param *fp;
	unsigned long vaddr = 0, paddr = 0;
	unsigned long len = 0;
	int w, h, pixb, bcnt = 1;
	int xp, yp;
	int fd;

	fd = fb_open(path);
	if (0 > fd)
		return NULL;

	fp = (struct fb_param *)malloc(sizeof(*fp));
	if (!fp) {
		printf("failed malloc size %zu, %s\n",
				sizeof(*fp), strerror(errno));
		return NULL;
	}
	memset((void*)fp, 0, sizeof(*fp));

	if (0 > fb_mmap(fd, &vaddr, &paddr, &len))
		goto err_fb;

	if (0 > fb_get_resol(fd, &w, &h, &pixb, &bcnt, &xp, &yp))
		goto err_fb;

	fp->fd = fd;
	fp->base = vaddr;
	fp->length = len;
	fp->xresol = w;
	fp->yresol = h;
	fp->pixelbyte = pixb;
	fp->buffers = bcnt;
	fp->x_pitch = xp;
	fp->y_pitch = yp;

	printf("%s: 0x%lx(0x%lx), %8ld (%4d * %4d : %5d * %5d, %d bpp, %d bn)\n",
		path, vaddr, paddr, len, w, h,
		fp->x_pitch, fp->y_pitch, pixb*8, bcnt);

	return fp;

err_fb:
	if (vaddr)
		fb_munmap(vaddr, len);

	if (fd >= 0)
		fb_close(fd);

	if (fp)
		free(fp);

	return NULL;
}

static void fb_release(struct fb_param *fp)
{
	unsigned long base, length;
	int fd;
	__assert__(fp);

	fd = fp->fd;
	base = fp->base;
	length = fp->length;

	if (0 > fd)
		goto exit_out;

	fb_flip(fd, 0);	/* restore fp */

	if (base)
		fb_munmap(base, length);

	fb_close(fd);

exit_out:
	if (fp)
		free(fp);
}

static int fb_change_resol(char *path,
			int sx, int sy, int width, int height, int pixelbyte)
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

static void fb_load_bmp(struct fb_param *fp, int sx, int sy, const char *bmp)
{
	unsigned long base;
	int xres, yres, pixb;
	void *ptr;

	if (sx > fp->xresol || sy > fp->yresol) {
		printf("over sx %d, sy %d (resol %d x %d) \n",
			sx, sy, fp->xresol, fp->yresol);
		return;
	}

	base = fp->base;
	xres = fp->xresol;
	yres = fp->yresol;
	pixb = fp->pixelbyte;

	printf("bmp: %s to %d, %d, %d, %d, %dpix\n",
		bmp, sx, sy, xres, yres, pixb);

	ptr = load_bmp2rgb(bmp, base, xres, yres, pixb);

	unload_bmp(ptr);
}

static void fb_draw_txt(struct fb_param *fp,
			const char *txt, int sx, int sy, int hscale, int vscale,
			unsigned text_color, unsigned int back_color,
			int alpha)
{
	unsigned long base;
	int xres, yres, pixb;
	int font = 0;

	if (sx > fp->xresol || sy > fp->yresol) {
		printf("over sx %d, sy %d (resol %d x %d) \n",
			sx, sy, fp->xresol, fp->yresol);
		return;
	}

	base = fp->base;
	xres = fp->xresol;
	yres = fp->yresol;
	pixb = fp->pixelbyte;

	printf("text: %s to %d,%d scale %d,%d txt 0x%x, back 0x%x, alpha %d\n",
		txt, sx, sy, hscale, vscale, text_color, back_color, alpha);

	draw_text(txt, sx, sy, hscale, vscale, alpha,
			text_color, back_color, font, base, xres, yres, pixb);
}

static void fb_fill_rect(struct fb_param *fp, int sx, int sy, int w, int h,
					unsigned int color, int random)
{
	unsigned long base;
	int xres, yres, pixb;
	int x1, y1, x2, y2;

	if (sx > fp->xresol || sy > fp->yresol ||
		w  > fp->xresol || h  > fp->yresol ) {
		printf("over sx %d, sy %d, w %d, h %d (resol %d x %d) \n",
			sx, sy, w, h, fp->xresol, fp->yresol);
		return;
	}

	base = fp->base;
	xres = fp->xresol;
	yres = fp->yresol;
	pixb = fp->pixelbyte;

	printf("fill: %d~%d, %d~%d (%dx%d, %dpix) 0x%08x [%s]\n",
		sx, w, sy, h, xres, yres, pixb, color, random?"random":"fill");

	if (!random) {
		fill_rgb(base, xres, yres, pixb, sx, sy, w, h, color);
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

		fill_rgb(base, xres, yres, pixb, x1, y1, x2, y2, color);
	}
}

static void fb_flip_pan(struct fb_param *fp, int align, int wait)
{
	unsigned long base;
	int xres, yres, pixb;
	int sx, sy, hs, vs;
	unsigned int front_color = 0xffffffff;
	unsigned int back_color = 0x0;
	int flip_count = 0, size;
	struct timeval ts, te;
	double t;
	int i = 0;

	if (2 > fp->buffers) {
		printf("fb buffers %d, not support flip pan display\n",
			fp->buffers);
		return;
	}

	xres = fp->xresol;
	yres = fp->yresol;
	pixb = fp->pixelbyte;
	base = fp->base;

	size = fp->x_pitch * fp->y_pitch * fp->pixelbyte;
	memset((void*)base, 0x0, size);

	sx = (xres / 10);
	sy = (yres / 10);
	hs = (sx * 8)/ 8;	/* x font size:  8 */
	vs = (sy * 8)/16;	/* y font size: 16 */

	/* framebuffer 0 */
	for (i = 0; fp->buffers > i; i++) {
		unsigned long addr;
		char s[16] = { 0, };

		sprintf(s, "%d", i);
		addr = base + (fp->xresol * fp->yresol * fp->pixelbyte) * i;

		if (align)
			 addr = FB_ALIGN(addr, align);

		draw_text(s, sx, sy, hs, vs, 0,
			front_color,
			back_color,
			0, addr, xres, yres, pixb);

		printf("[flip fb[%d]:0x%lx, offs:0x%lx, align:%d, wait:%d]\n",
			i, base, addr - base, align, wait);
	}

	while (1) {
		int nr = i++ % fp->buffers;

		if (!wait && flip_count == 0)
			gettimeofday(&ts, NULL);

		fb_flip(fp->fd, nr);
		flip_count++;

		if (wait)
			sleep(wait);

		if (!wait && flip_count == 60) {
			gettimeofday(&te, NULL);
			t = te.tv_sec + te.tv_usec * 1e-6 -
				(ts.tv_sec + ts.tv_usec * 1e-6);
			printf("freq: %.02fHz\n", flip_count / t);
			flip_count = 0;
			ts = te;
		}
	}
}

void print_usage(void)
{
	printf("\n");
    printf("usage: options\n");
    printf("-d framebuffer node path \n");
    printf("-f color  fill framebuffer with input color (hex)\n");
	printf("-r random fill framebuffer \n");
    printf("-p fill framebuffer in position\n");
    printf("   options [startx],[starty],[width],[height]\n");
	printf("-b load bmp to framebuffer\n");
	printf("-s draw input text with option '-t'\n");
	printf("-t draw text config\n");
	printf("   options [startx],[starty],[horizontal scale],[vertical scale],[text color],[back color],[alpha]\n");
	printf("-i pan display\n");
	printf("   options [align] [wait]\n");
	printf("-c change resolution, *** note> support private IoCtl\n");
	printf("   options [width],[height],[pixelbyte]\n");
	printf("-m move frame buffer, *** note> support private IoCtl\n");
	printf("   options [startx],[starty]\n");
}

int main(int argc, char **argv)
{
	int opt;
	struct fb_param *fp = NULL;
	char *path = FB_DEV_PATH;
	char *c;

	char *bmp = NULL, *text = NULL;
	unsigned int color = FILL_COLOR;
	int sx = 0, sy = 0, w = 0, h = 0, pixelbyte = 0;
	int hs = 4, vs = 4, alpha = 0;
	unsigned text_color = FILL_COLOR, back_color = 0x0;

	int o_random = 0, o_resol = 0, o_move = 0;
	int o_flip = 0;
	int ret = 0;

#define	conv_opt(p, v, t)	{	\
	v = strtol(p, NULL, t), p = strchr(p, ',');	\
	if (!p)	\
		break;	\
	p++;	\
	}

    while (-1 != (opt = getopt(argc, argv, "hd:f:p:rb:s:t:c:m:i"))) {
		switch(opt) {
        case 'h':
        		print_usage();
        		exit(0);
        		break;

		case 'd':
				path = optarg;
				break;

		case 'f':
			   color = strtoul(optarg, NULL, 16);
			   break;

		case 'r':
			   o_random = 1;
			   break;

		case 's':
			   text = optarg;
			   break;

		case 'i':
			   o_flip = 1;
			   break;

		case 't':
				c = optarg;
				conv_opt(c, sx, 10);
				conv_opt(c, sy, 10);
				conv_opt(c, hs, 10);
				conv_opt(c, vs, 10);
				conv_opt(c, text_color, 16);
				conv_opt(c, back_color, 16);
				conv_opt(c, alpha, 10);
				break;

		case 'b':
				bmp = optarg;
				break;

		case 'p':
				c = optarg;
				conv_opt(c, sx, 10);
				conv_opt(c, sy, 10);
				conv_opt(c,  w, 10);
				conv_opt(c,  h, 10);
				break;

		case 'c':
				o_resol = 1;
				c = optarg;
				conv_opt(c, w, 10);
				conv_opt(c, h, 10);
				conv_opt(c, pixelbyte, 10);
				break;

		case 'm':
				o_move = 1;
				c = optarg;
				conv_opt(c, sx, 10);
				conv_opt(c, sy, 10);
				break;

		default:
				print_usage(), exit(0);
				break;
		};
	}

	if (o_resol) {
		if (0 > fb_change_resol(path, 0, w, 0, h, pixelbyte))
			return ret;
	}

	if (o_move) {
		if (0 > fb_change_pos(path, sx, sy))
			return ret;
	}

	register_signal();

	fp = fb_initialize(path);
	if (!fp)
		return 0;

	__fb_parm = fp;

	if (o_flip) {
		int align = 0, wait = 0;

		if (argc > optind)
			align = strtol(argv[optind++], NULL, 10);

		if (argc > optind)
			wait = strtol(argv[optind++], NULL, 10);

		fb_flip_pan(fp, align, wait);
		return 0;
	}

	w = w ? : fp->xresol;
	h = h ? : fp->yresol;

	if (bmp)
		fb_load_bmp(fp, sx, sy, bmp);
	else
		fb_fill_rect(fp, sx, sy, w, h, color, o_random);

	if (text)
		fb_draw_txt(fp, text, sx, sy, hs, vs,
			text_color, back_color, alpha);

	fb_release(fp);

	return 0;
}
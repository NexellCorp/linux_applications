
#include <stdio.h>
#include <string.h>

#ifndef __FB_DEV__
#define	__FB_DEV__

#define	__DEBUG__

#ifdef 	__DEBUG__
#define	__break__		 do { } while (1)
#define __assert__(expr) do {	\
		if (!(expr)) {          		\
			printf("%s(%d) : %s (%s)\n",	\
				__FILE__, __LINE__, "_ASSERT", #expr);	\
			__break__;                         			\
		}                                 	\
	} while (0)
#else
#define __assert__(expr)
#define __break__
#endif

/*-----------------------------------------------------------------------------
 * Frame buffer APIs
 -----------------------------------------------------------------------------*/

int  fb_open(const char *name);
void fb_close(int fd);
int  fb_mmap(int fd, unsigned long *vaddr, unsigned long *paddr, unsigned long *len);
void fb_munmap(unsigned long base, unsigned long len);
int  fb_get_resol(int fd, int *width, int *height, int *pixbyte, int *buffcnt,
			int *x_pitch, int *y_pitch);
int  fb_set_resol(int fd, int startx, int starty, int width, int height, int pixbyte);
int  fb_pos(int fd, int startx, int starty);
void fb_flip(int fd, int buffno);

#endif

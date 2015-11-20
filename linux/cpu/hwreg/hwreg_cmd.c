#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h> 			/* O_RDWR */
#include <unistd.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/ioctl.h> 		/* ioctl */
#include <errno.h> 			/* error */
#include <sys/mman.h> 		/* for mmap */
#include <termios.h>		/* getch */
#include <stdbool.h>

struct iomap_table {
	unsigned int phys;
	unsigned int virt;
	unsigned int size;
};

struct iomap_table io_table [] =
{
	{ .phys = 0xC0000000, .size = 0x00300000, },
	{ .phys = 0xF0000000, .size = 0x00100000, },
	{ .phys = 0xCF000000, .size = 0x00100000, },
	{ .phys = 0x2C000000, .size = 0x00100000, },
	{ .phys = 0x40000000, .size = 0x01000000, },
//	{ .phys = 0xFFFF0000, .size = 0x00100000, },
};
#define	TABLE_SIZE		((int)(sizeof(io_table)/sizeof(io_table[0])))

#define	IO_PHYS(p, i)	((&p[i])->phys)
#define	IO_VIRT(p, i)	((&p[i])->virt)
#define	IO_SIZE(p, i)	((&p[i])->size)

static unsigned int iomem_map (unsigned int phys, unsigned int len);
static void 		iomem_free(unsigned int virt, unsigned int len);

void print_usage(void)
{
    printf( "usage: options\n"
            "-a	address (hex), 0xC0000000 ~		\n"
            "-w	write data to register (hex)	\n"
            "-l	read length (hex)				\n"
            );
}

int main(int argc, char **argv)
{
	struct iomap_table *io = io_table;
	unsigned int virt = 0, phys = 0, size = 0;
	unsigned int ioaddr = 0, iodata = 0;
	int length = 4, i = 0;
	int opt, opt_w = 0;

	for (i = 0; TABLE_SIZE > i; i++) {
		IO_VIRT(io,i) = iomem_map(IO_PHYS(io,i), IO_SIZE(io,i));
		if (! IO_VIRT(io,i))
			printf("Fail: mmap phys 0x%08x, length %d\n", IO_PHYS(io,i), IO_SIZE(io,i));
	}

    while (-1 != (opt = getopt(argc, argv, "ha:w:l:"))) {
		switch(opt) {
        case 'a':   ioaddr = strtoul(optarg, NULL, 16); break;
        case 'w':   iodata = strtoul(optarg, NULL, 16);
        			opt_w = 1;	break;
        case 'l':   length = strtoul(optarg, NULL, 16);
        			opt_w = 0;	break;
        case 'h':   print_usage();
        			exit(0);
        default:
        	break;
         }
	}

	/* check input params */
	if (! ioaddr) {
    	printf("Fail, no address \n");
    	print_usage();
		goto _exit;
	}

	/* align */
	ioaddr &= ~(0x3);

	for (i = 0; TABLE_SIZE > i; i++) {
		if ((IO_PHYS(io,i)+IO_SIZE(io,i)) > ioaddr &&
			ioaddr >= IO_PHYS(io,i)) {
			phys = IO_PHYS(io,i);
			virt = IO_VIRT(io,i);
			size = IO_SIZE(io,i);
			break;
		}
	}

	printf("support io table:\n");
	for (i = 0; TABLE_SIZE > i; i++)
		printf("io [%d] 0x%08x ~ 0x%08x : 0x%x\n",
			i, IO_PHYS(io,i), (IO_PHYS(io,i)+IO_SIZE(io,i)), IO_VIRT(io, i));

	if (!virt) {
		printf("Invalid address 0x%x\n", ioaddr);
		goto _exit;
	}

	if (opt_w) {
		printf("\n(W) 0x%08x: 0x%08x\n", ioaddr, iodata);
		*(unsigned int*)(virt + (ioaddr - phys)) = iodata;
	} else {

		if (4 > length)
			length = 4;

		if ((ioaddr+length) >= (phys+size)) {
			printf("Fail, 0x%x io range 0x%08x ~ 0x%x \n", ioaddr, phys, (phys+size));
			goto _exit;
		}

		for (i = 0 ; (length/4) > i; i++, ioaddr +=4) {
			if (0 == i%4)
				printf("\n(R) 0x%08x: ", ioaddr);
			printf("0x%08x ", *(uint *)(virt + (ioaddr - phys)));
		}
	}
	printf("\n");

_exit:
	for (i = 0; TABLE_SIZE > i; i++)
		iomem_free(IO_VIRT(io,i), IO_SIZE(io,i));

	return 0;
}


//------------------------------------------------------------------------------
#define	MMAP_ALIGN		4096	// 0x1000
#define	MMAP_DEVICE		"/dev/mem"

static unsigned int iomem_map(unsigned int phys, unsigned int len)
{
	unsigned int virt = 0;
	int fd;

	fd = open(MMAP_DEVICE, O_RDWR|O_SYNC);
	if (0 > fd) {
		printf("Fail, open %s, %s\n", MMAP_DEVICE, strerror(errno));
		return 0;
	}

	if (len & (MMAP_ALIGN-1))
		len = (len & ~(MMAP_ALIGN-1)) + MMAP_ALIGN;

	virt = (unsigned int)mmap((void*)0, len,
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)phys);
	if (-1 == (int)virt) {
		printf("Fail: map phys=0x%08x, len=%d, %s \n", phys, len, strerror(errno));
		goto _err;
	}

_err:
	close(fd);
	return virt;
}

static void iomem_free(unsigned int virt, unsigned int len)
{
	if (virt && len)
		munmap((void*)virt, len);
}

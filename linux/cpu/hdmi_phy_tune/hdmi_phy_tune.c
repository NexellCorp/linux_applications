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
#include <stdarg.h>
#include <errno.h>

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

#define OFF3C			(0x3C)
#define OFF40			(0x40)
#define OFF5C			(0x5C)

volatile unsigned int HDMI_PHY_REG_3C = 0xffffffff;
volatile unsigned int HDMI_PHY_REG_40 = 0xffffffff;
volatile unsigned int HDMI_PHY_REG_5C = 0xffffffff;

unsigned int virt = 0, phys = 0, size = 0;
int opt_d = 0;

static unsigned int iomem_map (unsigned int phys, unsigned int len);
static void 		iomem_free(unsigned int virt, unsigned int len);

void print_usage(void)
{
    printf( "usage: options\n"
			"-m	4418 or 6818  (default:4418)             \n"
			"-a	write TX_AMP_LVL value (0-31)            \n" 
			"-e	write TX_EMP_LVL value (0-15)            \n"
			"-c	write TX_CLK_LVL value (0-31)            \n"
			"\n"
			"ex) hdmi_phy_tune -m 6818 -a 16 -c 23       \n"
            );
}

int nx_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);

	if (opt_d == 1)
		vprintf(format, args);

    va_end(args);

	return 0;
}



unsigned int GET_AMP_LVL_BIT0(void)
{
	unsigned int pos = 7;
	unsigned int mask = (1<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_3C - phys));
	regval = (regval & mask) >> pos;
	return regval;
}

unsigned int GET_AMP_LVL_BIT4_1(void)
{
	unsigned int pos = 0;
	unsigned int mask = (0xf<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_40 - phys));
	regval = (regval & mask) >> pos;
	return regval;
}

unsigned int GET_AMP_LVL(void)
{
	return GET_AMP_LVL_BIT0() | (GET_AMP_LVL_BIT4_1()<<1);
}

int SET_AMP_LVL_BIT0(unsigned int val)
{
	unsigned int pos = 7;
	unsigned int mask = (1<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_3C - phys));
	nx_debug("read 3C: 0x%x\n", regval);

	regval &= ~(mask);
	regval |= (val & 0x1)<<pos;

	nx_debug("write 3C: 0x%x\n", regval);
	*(unsigned int*)(virt + (HDMI_PHY_REG_3C - phys)) = regval;

	return 0;
}

int SET_AMP_LVL_BIT4_1(unsigned int val)
{
	unsigned int pos = 0;
	unsigned int mask = (0xf<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_40 - phys));
	nx_debug("read 40: 0x%x\n", regval);

	regval &= ~(mask);
	regval |= ((val&0x1e) >> 1)<<pos;

	nx_debug("write 40: 0x%x\n", regval);
	*(unsigned int*)(virt + (HDMI_PHY_REG_40 - phys)) = regval;

	return 0;
}

int SET_AMP_LVL(unsigned int val)
{
	SET_AMP_LVL_BIT0(val);
	SET_AMP_LVL_BIT4_1(val);

	return 0;
}


unsigned int GET_CLK_LVL(void)
{
	unsigned int pos = 3;
	unsigned int mask = (0x1f<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_5C - phys));
	regval = (regval & mask) >> pos;
	return regval;
}

int SET_CLK_LVL(unsigned int val)
{
	unsigned int pos = 3;
	unsigned int mask = (0x1f<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_5C - phys));
	nx_debug("read 5C: 0x%x\n", regval);

	regval &= ~(mask);
	regval |= ((val&0x1f))<<pos;

	nx_debug("write 5C: 0x%x\n", regval);
	*(unsigned int*)(virt + (HDMI_PHY_REG_5C - phys)) = regval;

	return 0;
}


unsigned int GET_EMP_LVL(void)
{
	unsigned int pos = 4;
	unsigned int mask = (0xf<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_40 - phys));
	regval = (regval & mask) >> pos;
	return regval;
}

int SET_EMP_LVL(unsigned int val)
{
	unsigned int pos = 4;
	unsigned int mask = (0xf<<pos);
	volatile unsigned int regval = 0;

	regval = *(volatile uint *)(virt + (HDMI_PHY_REG_40 - phys));
	nx_debug("read 40: 0x%x\n", regval);

	regval &= ~(mask);
	regval |= ((val&0xf))<<pos;

	nx_debug("write 40: 0x%x\n", regval);
	*(unsigned int*)(virt + (HDMI_PHY_REG_40 - phys)) = regval;

	return 0;
}





int main(int argc, char **argv)
{
	struct iomap_table *io = io_table;
	unsigned int ioaddr = 0;
	unsigned int tx_a_lvl = 0, tx_e_lvl = 0, tx_c_lvl = 0;;
	int i = 0 , err = 0;
	int opt, opt_a = 0, opt_e = 0, opt_c = 0;
	int mach = 4418;
	unsigned int HDMI_PHY_REG_BASE = 0xffffffff;

	for (i = 0; TABLE_SIZE > i; i++) {
		IO_VIRT(io,i) = iomem_map(IO_PHYS(io,i), IO_SIZE(io,i));
		if (! IO_VIRT(io,i))
			printf("Fail: mmap phys 0x%08x, length %d\n", IO_PHYS(io,i), IO_SIZE(io,i));
	}

    while (-1 != (opt = getopt(argc, argv, "hm:a:e:c:d"))) {
		switch(opt) {
		case 'm':	mach = strtoul(optarg, NULL, 10);	break;
        case 'a':   tx_a_lvl = strtoul(optarg, NULL, 10);
        			opt_a = 1;	break;
		case 'e':   tx_e_lvl = strtoul(optarg, NULL, 10);
					opt_e = 1;	break;
		case 'c':	tx_c_lvl = strtoul(optarg, NULL, 10);
					opt_c = 1;	break;
		case 'd':	opt_d = 1;  break;
        case 'h':   print_usage();
        			exit(0);
        default:
        	break;
         }
	}



	if (opt_a)
		if (tx_a_lvl > 31) {
			printf("error: AMP range\n");
			err = 1;
		}
	if (opt_e)
		if (tx_e_lvl > 15) {
			printf("error: EMP range\n");
			err = 1;
		}
	if (opt_c)
		if (tx_c_lvl > 31) {
			printf("error: CLK range\n");
			err = 1;
		}


	if (mach == 6818)
		HDMI_PHY_REG_BASE = 0xc00f0000;
	else if (mach == 4418)
		HDMI_PHY_REG_BASE = 0xc0100000 + 0x400;
	else {
		printf("error: machine select\n");
		err = 1;
	}

	if (err)
		return (-EINVAL);



	/* HDMI REG addr setting */
	HDMI_PHY_REG_3C = HDMI_PHY_REG_BASE + OFF3C;
	HDMI_PHY_REG_40 = HDMI_PHY_REG_BASE + OFF40;
	HDMI_PHY_REG_5C = HDMI_PHY_REG_BASE + OFF5C;

	ioaddr = HDMI_PHY_REG_BASE;





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

	if (!virt) {
		printf("support io table:\n");
		for (i = 0; TABLE_SIZE > i; i++)
			printf("io [%d] 0x%08x ~ 0x%08x\n",
				i, IO_PHYS(io,i), (IO_PHYS(io,i)+IO_SIZE(io,i)));
		goto _exit;
	}



	printf("Current :\n\t[AMP] %02d  [EMP] %02d  [CLK] %02d\n",
		GET_AMP_LVL(), GET_EMP_LVL(), GET_CLK_LVL());

	/* tx amp lvl */
	if (opt_a) {
		printf("  setting amp level...\n");
		SET_AMP_LVL(tx_a_lvl);
	}
	if (opt_e) {
		printf("  setting emp level...\n");
		SET_EMP_LVL(tx_e_lvl);
	}
	if (opt_c) {
		printf("  setting clk level...\n");
		SET_CLK_LVL(tx_c_lvl);
	}

	printf("Current :\n\t[AMP] %02d  [EMP] %02d  [CLK] %02d\n",
		GET_AMP_LVL(), GET_EMP_LVL(), GET_CLK_LVL());

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

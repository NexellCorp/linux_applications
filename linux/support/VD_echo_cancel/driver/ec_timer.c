/*
 * (C) Copyright 2009
 * jung hyun kim, Nexell Co, <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <asm/io.h>

#include <mach/platform.h>

#define	EC_TIMER_NAME		"ec-timer"
#define	EC_TIMER_CHANNEL	3
#define	EC_TIMER_TCOUNT		20
#define	EC_TIMER_PCLK		200000000					// 204.8 MHZ (204800000)
#define	EC_TIMER_HZ_NS		(1000000000/EC_TIMER_PCLK)	// 204.8 MHZ (204800000)

/* Timer HW */
#define	__TIMER_BASE	IO_ADDRESS(PHY_BASEADDR_TIMER)
#define	TIMER_CFG0		(__TIMER_BASE + 0x00)
#define	TIMER_CFG1		(__TIMER_BASE + 0x04)
#define	TIMER_TCON		(__TIMER_BASE + 0x08)
#define	TIMER_STAT		(__TIMER_BASE + 0x44)
#define	TIMER_CNTB(ch)	((__TIMER_BASE + 0x0C) + (0xc*ch))
#define	TIMER_CMPB(ch)	((__TIMER_BASE + 0x10) + (0xc*ch))
#define	TIMER_CNTO(ch)	((__TIMER_BASE + 0x14) + (0xc*ch))

#define	TCON_AUTO		(1<<3)
#define	TCON_INVT		(1<<2)
#define	TCON_UP			(1<<1)
#define	TCON_RUN		(1<<0)
#define TCFG0(c)		(c == 0 || c == 1 ? 0 : 8)
#define TCFG1(c)		(c * 4)
#define TCON(c)		(c ? c * 4  + 4 : 0)
#define TINTON(c)		(c)
#define TINTST(c)		(c + 5)
#define	TINTMASK		(0x1F)

#define	T_MUX_1_1		0
#define	T_MUX_1_2		1
#define	T_MUX_1_4		2
#define	T_MUX_1_8		3
#define	T_MUX_1_16		4
#define	T_MUX_TCLK		5

#define	TIMER_INT_ON	(1<<0)

static inline void timer_clock(int ch, int mux, int scl)
{
	__raw_writel((__raw_readl(TIMER_CFG0) & ~(0xFF << TCFG0(ch))) |
			((scl-1)<< TCFG0(ch)), TIMER_CFG0);
	__raw_writel((__raw_readl(TIMER_CFG1) & ~(0xF << TCFG1(ch))) |
			(mux << TCFG1(ch)), TIMER_CFG1);
}

static inline void timer_count(int ch, unsigned int cnt)
{
	__raw_writel(cnt, TIMER_CNTB(ch)), __raw_writel(cnt, TIMER_CMPB(ch));
}

static inline void timer_start(int ch, unsigned int flag)
{
	u32 val = (__raw_readl(TIMER_STAT) & ~(TINTMASK<<5 | 0x1 << TINTON(ch))) |
		   (0x1 << TINTST(ch) | ((flag|TIMER_INT_ON)?1:0) << TINTON(ch));
	__raw_writel(val, TIMER_STAT);

	val = (__raw_readl(TIMER_TCON) & ~(0xE << TCON(ch))) | (TCON_UP << TCON(ch));
	__raw_writel(val, TIMER_TCON);

	val = (val & ~(TCON_UP << TCON(ch))) | ((TCON_AUTO | TCON_RUN) << TCON(ch));
	__raw_writel(val, TIMER_TCON);
}

static inline void timer_stop(int ch, unsigned int flag)
{
	u32 val = (__raw_readl(TIMER_STAT) & ~(TINTMASK<<5 | 0x1 << TINTON(ch))) |
		   (0x1 << TINTST(ch) | ((flag|TIMER_INT_ON)?1:0) << TINTON(ch));
	__raw_writel(val, TIMER_STAT);

	val = __raw_readl(TIMER_TCON) & ~(TCON_RUN << TCON(ch));
	__raw_writel(val, TIMER_TCON);
}

static inline void timer_int_clear(int ch)
{
	u32 val = (__raw_readl(TIMER_STAT) & ~(TINTMASK<<5)) | (0x1 << TINTST(ch));
	__raw_writel(val, TIMER_STAT);
}

/* TIMER DRIVER */
static struct timespec tv_sp;

void ec_timer_read(struct timespec *t)
{
	struct timespec *ts = &tv_sp;
	int ch = EC_TIMER_CHANNEL;

	unsigned long cnt = EC_TIMER_PCLK - __raw_readl(TIMER_CNTO(ch));
	unsigned long ns = EC_TIMER_HZ_NS;
	unsigned long nsec = cnt * ns;

	t->tv_sec  = ts->tv_sec;
	t->tv_nsec = nsec;

	return;
}

static irqreturn_t ec_timer_handler(int irq, void *desc)
{
	struct timespec *ts = desc;
	int ch = EC_TIMER_CHANNEL;

	unsigned long cnt = __raw_readl(TIMER_CNTO(ch));
	unsigned long ns = (EC_TIMER_PCLK - cnt) * EC_TIMER_HZ_NS;

	timer_int_clear(ch);

	ts->tv_sec += 1;
	ts->tv_nsec = ns;

	printk("[%6ld.%09ld](%ld)\n", ts->tv_sec, ts->tv_nsec, cnt);

	return IRQ_HANDLED;
}

static int ec_timer_probe(struct platform_device *pdev)
{
	struct timespec *ts = &tv_sp;
	int irq, ch = EC_TIMER_CHANNEL;
	int mux = T_MUX_1_1;
	int prescale = 1;
	unsigned int flag = TIMER_INT_ON;
	unsigned int count = EC_TIMER_PCLK;
	int ret = 0;

	ts->tv_sec  = 0;
	ts->tv_nsec = 0;

	irq = IRQ_PHY_TIMER_INT0 + ch;
	ret = request_irq(irq, &ec_timer_handler,
				IRQF_DISABLED, EC_TIMER_NAME, &tv_sp);
	if (ret) {
		printk("Error: EC timer.%d request interrupt %d \n", ch, irq);
		return ret;
	}

	timer_stop (ch, 1);
	timer_clock(ch, mux, prescale);
	timer_count(ch, count);
	timer_start(ch, flag);

	return 0;
}

static struct platform_driver ec_tm_driver = {
	.driver	= {
	.name	= EC_TIMER_NAME,
	.owner	= THIS_MODULE,
	},
	.probe	= ec_timer_probe,
};
static struct platform_device *pd;

static int __init ec_timer_init(void)
{
    if (platform_driver_register(&ec_tm_driver))
        return -EINVAL;

	pd = platform_device_register_simple(EC_TIMER_NAME, -1, NULL, 0);
    if (IS_ERR(pd)) {
        platform_driver_unregister(&ec_tm_driver);
        return PTR_ERR(pd);
    }
    return 0;
}
module_init(ec_timer_init);

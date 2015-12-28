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

#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <mach/platform.h>
#include <mach/pdm.h>

#include "nxp-pcm-sync.h"


#define pr_debug		printk


struct snd_dev_info {
	bool is_run;
	void *base;
};

#define SND_SYNC_DEV_MASK		(	\
			(1<<SND_DEVICE_I2S0) |	\
			(1<<SND_DEVICE_I2S1) |	\
			(1<<SND_DEVICE_PDM)	\
			)
#define	SND_SYNC_DEV_NUM	4

static struct snd_dev_info dev_info[SND_SYNC_DEV_NUM];
static DEFINE_SPINLOCK(pcm_sync_lock);

static inline void __pdm_capture_start(void *base)
{
	unsigned int ctrl;
	pr_debug("%s (%p)\n", __func__, base);

	NX_PDM_ClearInterruptPendingAll(0);
	ctrl = __raw_readl(base) | (0x1<<1);
	__raw_writel(ctrl, base);
}

static inline void __i2s_capture_start(void *base, int status)
{
	unsigned int CON;	///< 0x00 :
	unsigned int CSR;	///< 0x04 :
	unsigned int FIC;	///< 0x08 : flush fifo

	pr_debug("%s (%p)\n", __func__, base);

	FIC  = __raw_readl(base + 0x08) | (1 << 7);
	CON  = __raw_readl(base + 0x00) | ((1 << 1) | (1 << 0));
	CSR  = (__raw_readl(base + 0x04) & ~(3 << 8)) | (1 << 8);
	CSR |= (SNDDEV_STATUS_PLAY & status ? (2 << 8) : 0);

	__raw_writel(FIC, (base+0x08));	/* Flush the current FIFO */
	__raw_writel(0x0, (base+0x08));	/* Clear the Flush bit */
	__raw_writel(CSR, (base+0x04));
	__raw_writel(CON, (base+0x00));
}

int nxp_snd_dev_trigger(struct snd_pcm_substream *substream,
					int cmd, enum snd_pcm_dev_type type,
					struct nxp_pcm_dma_param *dma_param, void *base, int status)
{
	struct snd_dev_info *pdev = dev_info;
	unsigned long mask = 0, flags;
	int i = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		return -EINVAL;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_START:

		spin_lock_irqsave(&pcm_sync_lock, flags);

		pdev[type].is_run = true;
		pdev[type].base = base;

		for (i = 0 ; SND_SYNC_DEV_NUM > i; i++)
			mask |= dev_info[i].is_run ? (1<<i): (0<<i);

		if (SND_SYNC_DEV_MASK != mask) {
			spin_unlock_irqrestore(&pcm_sync_lock, flags);
			break;
		}

		printk("***** RUN PCM CAPTURE SYNC (I2S/PDM) *****\n");

		for (i = 0; SND_SYNC_DEV_NUM > i; i++) {
			if (!pdev[i].is_run)
				continue;

			pdev[i].is_run = false;
			switch (i) {
			case SND_DEVICE_PDM:
				__pdm_capture_start(pdev[i].base);
				break;
			case SND_DEVICE_I2S0:
			case SND_DEVICE_I2S1:
			case SND_DEVICE_I2S2:
				__i2s_capture_start(pdev[i].base, status);
				break;
			}
		}

		spin_unlock_irqrestore(&pcm_sync_lock, flags);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

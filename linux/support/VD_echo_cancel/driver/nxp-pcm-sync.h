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

#include "nxp-pcm.h"

#ifndef __NXP_DEV_SYNC_H__
#define __NXP_DEV_SYNC_H__

enum snd_pcm_dev_type {
	SND_DEVICE_I2S0,	/* 1st */
	SND_DEVICE_I2S1,
	SND_DEVICE_I2S2,
	SND_DEVICE_PDM,		/* last */
};

extern int nxp_snd_dev_trigger(struct snd_pcm_substream *substream,
					int cmd, enum snd_pcm_dev_type type,
					struct nxp_pcm_dma_param *dma_param, void *base, int status);
#endif
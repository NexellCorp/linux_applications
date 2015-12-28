#
# Copyright 2009 Nexell Co.
#
# Makefile for the S5P4418

obj-y					:= nxp-pcm-sync.o resample.o resample2.o

obj-$(CONFIG_SND_NXP_SOC)	  		+= nxp-snd-pcm.o
obj-$(CONFIG_SND_NXP_I2S)	  		+= nxp-snd-i2s.o
obj-$(CONFIG_SND_NXP_PDM)	 		+= nxp-snd-pdm.o
obj-$(CONFIG_SND_PDM_REC)			+= nxp-snd-pdm-record.o
obj-$(CONFIG_SND_CODEC_NULL) 		+= nxp-snd-null.o
obj-$(CONFIG_SND_CODEC_RT5631)		+= nxp-snd-rt5631.o

nxp-snd-pcm-objs		:= nxp-pcm.o
nxp-snd-i2s-objs 		:= nxp-i2s.o
nxp-snd-pdm-objs		:= nxp-pdm.o

nxp-snd-pdm-record-objs := nxp-pdm-record.o
nxp-snd-null-objs 		:= nxp-null.o
nxp-snd-rt5631-objs 	:= nxp-rt5631.o



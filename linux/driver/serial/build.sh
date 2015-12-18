#!/bin/sh

make clean 
make CROSS_COMPILE=arm-generic-linux-gnueabi-
cp serial ~/devel/nfs/rootfs-dtk/root
#make clean 
#make



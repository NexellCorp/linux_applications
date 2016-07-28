
CROSS_COMPILE := arm-linux-gnueabihf-
ARCH := arm

#CROSS_COMPILE := aarch64-linux-gnu-
#ARCH := arm64

CC  = $(CROSS_COMPILE)gcc
CFLAGS  :=  -Wall -O3

LIBS 	+=
CFLAGS  += -O -W
LDFLAGS += -static

COBJS	:= fb_test.o fb_dev.o fb_draw.o
TARGET 	:= fb_test

OBJS	:= $(COBJS)

$(TARGET) : depend $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)


clean :
	rm -rf $(OBJS) $(TARGET) core .depend *.a

ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c)
depend dep:
	$(CC)  -M $(CFLAGS) $(SRCS) > .depend

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<


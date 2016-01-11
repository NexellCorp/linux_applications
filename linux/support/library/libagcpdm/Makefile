#
# For debug build
#	> make DEBUG=y
#
#

CROSS_COMPILE := arm-linux-gnueabihf-
CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++

CFLAGS  :=  -Wall -std=c++11
CFLAGS  +=  -mfloat-abi=hard
ARFLAGS	:= crv

ifeq ($(DEBUG),y)
CFLAGS  += -g
CFLAGS  += -fno-omit-frame-pointer
CFLAGS  += -fstack-protector-all
LDFLAGS += -rdynamic # '-rdynamic' or '-Wl,export-dynamic'
else
CFLAGS  += -O3
CFLAGS  += -fomit-frame-pointer
endif

CPPOBJS	:= nx_agc_new.o nx_pdm.o PDM_LPF.o
LIBS	+= -lm
OBJS	:= $(COBJS) $(CPPOBJS)

TARGET = libagcpdm

all : $(TARGET)

$(TARGET) : depend $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET).a $(OBJS)

.PHONY: clean

clean :
	rm -rf $(OBJS) $(TARGET) core .depend *.a

ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
depend dep:
	$(CC)  -M $(CFLAGS) $(SRCS) > .depend

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) $(INCLUDE) -c -o $@ $<

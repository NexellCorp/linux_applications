ifndef TOPDIR
TOPDIR := ./
endif

include $(TOPDIR)/env.mk

######################################################################
# Build options
INCLUDE += -I./

LIBRARY += -lpthread -lsupc++

CFLAGS 	:= 
CFLAGS 	:= -static

######################################################################
# Target
TARGET	:= iAPTest

######################################################################
# Build
COBJS	:=

CPPOBJS	:= 
CPPOBJS	+= main.o iPod.o Cpchip.o gpio.o
OBJS	:= $(COBJS) $(CPPOBJS)

all: $(TARGET)

clean :
	rm -f $(TARGET) $(UPDATE) *.o .depend

$(TARGET): depend $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBRARY)
	@echo "done"

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif
SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC)  -M  $(CFLAGS)   $(INCS) $(SRCS) > .depend

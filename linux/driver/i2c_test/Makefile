ifndef TOPDIR
TOPDIR := ./
endif

include $(TOPDIR)/env.mk

######################################################################
# Build options
INCLUDE += -I./

LIBRARY +=

CFLAGS 	:= 
CFLAGS 	:= -static

######################################################################
# Target
TARGET	:= i2c_test

######################################################################
# Build
COBJS	:= i2c_test.o

CPPOBJS	:= 
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

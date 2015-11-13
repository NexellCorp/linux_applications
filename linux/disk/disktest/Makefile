.SUFFIXES : .c .o

CROSS_COMPILE := arm-linux-gnueabihf-
#CROSS_COMPILE := aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

INC =
LIBS =
CFLAGS = $(INC) -O2
LDFLAGS = -static

OBJS = disk_test.o
SRCS = 

TARGET = disk_test


.PHONY: clean

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

dep :
	gccmakedep $(INC) $(SRCS)

clean :
	rm -rf $(OBJS) $(TARGET) core

new : 
	$(MAKE) clean 
	$(MAKE) 


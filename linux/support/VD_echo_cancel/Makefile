
CROSS_COMPILE := arm-linux-gnueabihf-
CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++

CFLAGS =  -W -mfloat-abi=hard -O3 -std=c++11
LDFLAGS = -static

COBJS	:=
CPPOBJS	:= ec_test.o audioplay.o nx_agc_new.o PDM_LPF.o
LIBS	:= libPreproc1.a libtinyalsa.a
LIBS	+= -lpthread -lm
OBJS	:= $(COBJS) $(CPPOBJS)

TARGET = ec_test

all : $(TARGET)

$(TARGET) : depend $(OBJS)
	$(CPP) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

.PHONY: clean

clean :
	rm -rf $(OBJS) $(TARGET) core .depend

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
depend dep:
	$(CC) -M $(CFLAGS) $(SRCS) > .depend

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

%.o: %.cpp
	$(CPP) $(CFLAGS) $(INCLUDE) -c -o $@ $<
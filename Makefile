###########################################
# Simple Makefile for HIDAPI test program
#
# Alan Ott
# Signal 11 Software
# 2010-07-03
###########################################

all: msiledenabler

CC=gcc
CXX=g++
COBJS=hid.o
CPPOBJS=msiledenabler.o
OBJS=$(COBJS) $(CPPOBJS)
CFLAGS+=-Ihidapi -Wall -g -c 
LIBS=-framework IOKit -framework CoreFoundation


msiledenabler: $(OBJS)
	g++ -Wall -g $^ $(LIBS) -o msiledenabler

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -f *.o msiledenabler $(CPPOBJS)

.PHONY: clean

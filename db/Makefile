CC=g++
CYGNUS=y
CFLAGS= -c -g -O3 -Wall -fgnu-tm
INCLUDES=-I../
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)

ifeq ($(CYGNUS),y)
RUNTIME_DIR	= /home/yihan/cygnus/Cygnus
CFLAGS	+= -DCYGNUS -I$(RUNTIME_DIR)/include/ $(shell pkg-config --cflags libdpdk)
endif

all: $(SOURCES) $(OBJECTS)

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	$(RM) $(OBJECTS)


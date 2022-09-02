CC=g++
CYGNUS=y
CFLAGS=-std=c++11 -g -Wall -pthread -I./
LDFLAGS= -lpthread -ltbb -lhiredis -lmemcached
SUBDIRS=core db redis
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

ifeq ($(CYGNUS),y)
RUNTIME_DIR	= /home/yihan/cygnus/Cygnus
CFLAGS	+= -DCYGNUS -I$(RUNTIME_DIR)/include/ $(shell pkg-config --cflags libdpdk)
LDFLAGS	+= -L$(RUNTIME_DIR)/build -lcygnus $(shell pkg-config --libs libdpdk)
endif

all: $(SUBDIRS) $(EXEC)

$(SUBDIRS):
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC)


CC=g++
CYGNUS=y
CFLAGS= -Wall -Werror -O3 -g -fno-stack-protector -fno-omit-frame-pointer -fPIC -I./ -I/usr/include/
LDFLAGS= -L/usr/lib -L/home/libmemcached/libmemcached/.libs -lmemcached
SUBDIRS=core db redis
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

ifeq ($(CYGNUS),y)
RUNTIME_DIR	= /home/yihan/cygnus/Cygnus
CFLAGS	+= -DCYGNUS -I$(RUNTIME_DIR)/include/ $(shell pkg-config --cflags libdpdk)
LDFLAGS	+= -L$(RUNTIME_DIR)/build -lcygnus $(shell pkg-config --libs libdpdk)
endif

LDFLAGS	+= -lhiredis -lpthread

CXX_STANDARD	:= -std=gnu++11

all: $(SUBDIRS) $(EXEC)

$(SUBDIRS):
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $(CXX_STANDARD) $^ -o $@ $(LDFLAGS)

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC)


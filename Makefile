# $Id$

TOPDIR = .

PLATFORM = unix
PREFIX = /usr/local

DIRS = Server

FLAGS = PLATFORM=$(PLATFORM) PREFIX=$(PREFIX)

include $(TOPDIR)/Platform/$(PLATFORM).mk

.PHONY: all clean

all: $(DIRS)

$(DIRS)::
	$(MAKE) -C $@ $(FLAGS)

clean:
	for i in $(DIRS); do \
		$(MAKE) -C $$i $(FLAGS) clean ; \
	done

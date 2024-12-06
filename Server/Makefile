# $Id$

TOPDIR = ..

VERSION = 1.0

include $(TOPDIR)/Platform/$(PLATFORM).mk

CFLAGS += -DVERSION=\"$(VERSION)\" -DPREFIX=\"$(PREFIX)\"

OBJS = main.o y.tab.o lex.yy.o

.PHONY: all clean
.SUFFIXES: .c .o

all: pftpd$(EXEC)

y.tab.c: config.y
	$(YACC) $(YFLAGS) config.y

lex.yy.c: config.l y.tab.c
	$(LEX) $(LFLAGS) config.l

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

pftpd$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f pftpd pftpd.exe *.o y.tab.c y.tab.h lex.yy.c

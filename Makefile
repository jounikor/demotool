#
#
#

.PHOBY: all clean dist
.SUFFIXES: .o .c

CC := vc
MAKE := make
LDFLAGS :=
CFLAGS := +aos68k -cpu=68000 -c99 -Inetinclude: -I../inc -I../plugins
LIBDIR := 
LIBS :=
RM := delete

DATE := $(shell date)
DIST := $(subst :,-,$(DATE))


SRCS :=	plugin.c demotool.c
OBJS := plugin.o demotool.o

INCS := demotool.h plugins.h

PROG := pl

#
#
#

all:
	$(MAKE) -i -C plugins all
	$(MAKE) -i -C server all
#	$(MAKE) -i -C client all


dist:
	@echo $(DIST)
	lha -r a $(DIST).lha #?.c #?.h #?Makefile

clean:
	$(MAKE) -i -C plugins clean
	$(MAKE) -i -C server clean
#	$(MAKE) -i -C client clean



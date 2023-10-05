#
#
#

.PHOBY: all clean dist
.SUFFIXES: .o .c

OS := __AMIGA__
CC := vc
MAKE := make
LDFLAGS :=
CFLAGS := +aos68k -cpu=68000 -c99 -Inetinclude: -I../inc -I../plugins
LIBDIR := 
LIBS :=
RM := delete
EXES := exes

DATE := $(shell date)
DIST := $(subst :,-,$(DATE))

#
#

all:
	$(MAKE) -i -C plugins all
	$(MAKE) -i -C server all
	$(MAKE) -i -C client all OS=$(OS)


dist:
	@echo $(DIST)
	lha -r a "$(DIST).lha" #?.c #?.h #?Makefile

dist_exes:
	@echo $(DIST)
	lha -r a "$(EXES).lha" #?.exe


clean:
	$(MAKE) -i -C plugins clean
	$(MAKE) -i -C server clean
	$(MAKE) -i -C client clean



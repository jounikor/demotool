#
#
#

.PHOBY: all clean dist pl cl se gs
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
LHA  := $(shell which lha)

ifeq ($(OS),__AMIGA__)
	RM := delete
	WILD := '\#?'
else
	RM := rm
	WILD := *
endif

#
#

all:
	$(MAKE) -i -C plugins all
	$(MAKE) -i -C server all
	$(MAKE) -i -C client all OS=$(OS)
	$(MAKE) -i -C searcher all

pl:
	$(MAKE) -i -C plugins all

se:
	$(MAKE) -i -C server all

cl:
	$(MAKE) -i -C client all OS=$(OS)

gs:
	$(MAKE) -i -C searcher all OS=$(OS)


dist:
	@echo $(DIST)
	$(LHA) -r a "$(DIST).lha" $(WILD).c $(WILD).h $(WILD)Makefile  $(WILD).doc

dist_exes:
	@echo $(DIST)
	$(LHA) -r a "$(EXES).lha" $(WILD).exe


clean:
	$(MAKE) -i -C plugins clean
	$(MAKE) -i -C server clean
	$(MAKE) -i -C client clean OS=$(OS)
	$(MAKE) -i -C searcher clean OS=$(OS)



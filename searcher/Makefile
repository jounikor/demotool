#
#
#

.PHOBY: all clean dist
.SUFFIXES: .o .c

#OS := __AMIGA__
MAKE := make
LHA  := $(shell which lha)

ifeq ($(OS),__AMIGA__)
	CC := vc
	LDFLAGS := +kick13m -lamiga
	CFLAGS :=  +kick13m -sc -sd -O2 -cpu=68000 -c99 -Inetinclude: -D$(OS)
	LIBDIR := 
	LIBS :=
	RM := delete
	WILD := '\#?'
else
	CC := cc
	LDFLAGS :=
	CFLAGS := -I../inc
	LIBDIR := 
	LIBS :=
	RM := rm
	WILD := *
endif



DATE := $(shell date)
DIST := $(subst :,-,$(DATE))

SRCS :=	searcher.c iffhandle.c ilbmw.c
OBJS := searcher.o iffhandle.o ilbmw.o
INCS := searcher.h iffhandle.h 

PROG := gs.exe

#
#
#

all:
	$(MAKE) $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -c $? -o $@
	
$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(LDFLAGS) $(OBJS)

run:
	$(PROG)

dist:
	@echo $(DIST)
	$(LHA) -x a $(DIST).lha $(WILD).c $(WILD).h $(WILD)Makefile $(WILD).doc

clean:
	$(RM) $(WILD).o $(WILD).i $(PROG)


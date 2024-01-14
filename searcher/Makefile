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
	LDFLAGS := +aos68km -lamiga
	CFLAGS :=  +aos68km -sc -sd -cpu=68000 -c99 -Inetinclude: -D$(OS)
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

SRCS :=	searcher.c 
OBJS := searcher.o
INCS := searcher.h

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


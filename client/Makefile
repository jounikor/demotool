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
	LDFLAGS := +aos68k -lauto
	CFLAGS :=  +aos68k -sd -sc -O2 -cpu=68000 -c99 -Inetinclude: -I../inc -I../plugins -D$(OS)
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

SRCS :=	demotool.c 
OBJS := demotool.o
INCS := demotool.h

ifeq ($(OS),__AMIGA__)
	SRCS := $(SRCS) getopt_long.c
	OBJS := $(OBJS) getopt_long.o
	INCS := $(INCS) getopt.h
endif


PROG := cl.exe

#
#
#

all:
	$(MAKE) $(PROG)

%.o: %.c
	$(CC) $(CFLAGS) -c $? -o $@
	
$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(LDFLAGS) $(OBJS)

dist:
	@echo $(DIST)
	$(LHA) -x a $(DIST).lha $(WILD).c $(WILD).h $(WILD)Makefile

clean:
	$(RM) $(WILD).o $(WILD).i $(PROG)


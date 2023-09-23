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

PROG := pl.exe

#
#
#

all:
	#cd plugins/ && $(MAKE) && cd /
	$(MAKE) $(PROG)

plugin.o: plugin.c plugin.h
	$(CC) -c plugin.c $(CFLAGS) 
	
demotool.o: demotool.c demotool.h plugin.h
	$(CC) -c demotool.c $(CFLAGS) 



#%.o: %.c
#	$(CC) -c $? $(CFLAGS) 

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(LDFLAGS) $(OBJS)

dist:
	@echo $(DIST)
	lha -x a $(DIST).lha #?.c #?.h #?Makefile

clean:
	delete \#?.o \#?.i $(PROG)


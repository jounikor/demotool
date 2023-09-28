#
#
#
#
#

PLUGIN_LDOPTS := -nostdlib -lamiga -sc -sd
PLUGIN_COPTS := +aos68k -sc -sd  -O2 -Inetinclude: -I../inc -cpu=68000 -c99

LOADSEG_EXE := loadseg_plugin.exe
LOADSEG_SRC := loadseg_plugin.c utils.c
LOADSEG_OBJ := loadseg_plugin.o utils.o
LOADSEG_INC := loadseg_plugin.h utils.h

INTERNALLOADSEG_EXE := internalloadseg_plugin.exe
INTERNALLOADSEG_SRC := internalloadseg_plugin.c
INTERNALLOADSEG_OBJ := internalloadseg_plugin.o
INTERNALLOADSEG_INC := internalloadseg_plugin.h

ADF_EXE     := adf_plugin.exe
ADF_SRC     := adf_plugin.c
ADF_OBJ     := adf_plugin.o
ADF_INC     := adf_plugin.h

ADDR_EXE    := addr_plugin.exe
ADDR_SRC    := addr_plugin.c
ADDR_OBJ    := addr_plugin.o
ADDR_INC    := addr_plugin.h

PLUGIN_HEADER := plugin_header.o

# Make all targets..
all: $(INTERNALLOADSEG_EXE) $(LOADSEG_EXE) $(ADF_EXE) $(ADDR_EXE)  $(PLUGIN_HEADER)


# The generic plugin header for all plugins
%.o: %.c
	vc $(PLUGIN_COPTS) -c $< -o $@

$(LOADSEG_EXE): $(PLUGIN_HEADER) $(LOADSEG_OBJ)
	vc $(PLUGIN_LDOPTS) $(PLUGIN_HEADER) $(LOADSEG_OBJ) -o $(LOADSEG_EXE)

$(INTERNALLOADSEG_EXE): $(PLUGIN_HEADER) $(INTERNALLOADSEG_OBJ)
	vc $(PLUGIN_LDOPTS) $(PLUGIN_HEADER) $(INTERNALLOADSEG_OBJ) -o $(INTERNALLOADSEG_EXE)

$(ADF_EXE): $(PLUGIN_HEADER) $(ADF_OBJ)
	vc $(PLUGIN_LDOPTS) $(PLUGIN_HEADER) $(ADF_OBJ) -o $(ADF_EXE)

$(ADDR_EXE): $(PLUGIN_HEADER) $(ADDR_OBJ)
	vc $(PLUGIN_LDOPTS) $(PLUGIN_HEADER) $(ADDR_OBJ) -o $(ADDR_EXE)


# Cleanups

clean:
	delete \#?.lnk \#?.o

cleanall:
	delete \#?.lnk \#?.o \#?.exe



# ----------------------------------------------------------------------------
# file: Makefile
#
# makefile for ao software testing
#
# ----------------------------------------------------------------------------


CC	= gcc
AR	= ar
LD	= ld
RANLIB	= ranlib

MFLAGS	= -pipe -Wall -Wextra -Wshadow -lpthread  -std=c99
GFLAGS	= -g -DDEBUG_ITIFG=255 -pg
IFLAGS  = -I. -L.
LFLAGS  = -levent -lcfitsio -lm -lcfitsio -lgsl
SDLFLAGS = `sdl-config --libs --cflags`

CFLAGS	= $(MFLAGS) $(GFLAGS) $(IFLAGS) $(LFLAGS) $(SDLFLAGS)

# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
all:	proto-sim

clean:	
	-rm -f protocs-sim protoui

# make multi-file prototyped AO
# ----------------------------------------------------------------------------

proto-sim: protocs-sim protoui

protocs-sim: proto_ao_cs.c ao_library.c cs_library.c foam_modules-sim.c foam_modules-dm.c
		$(CC) $(CFLAGS) -lc $^ -o $@

protoui: proto_ao_ui.c ao_library.c ui_library.c
		$(CC) $(CFLAGS) -lc $^ -o $@



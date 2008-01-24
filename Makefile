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

MFLAGS	= -pipe -Wall -Wextra -lpthread  -std=c99 # -Wshadow
GFLAGS	= -g -DDEBUG_ITIFG=255 -finstrument-functions # -pg for gprof profiling,  -finstrument-functions for Saturn profiling
IFLAGS  = -I. -L.
OFLAGS  =  -O3 -ftree-vectorize # tree-vectorize works with gcc 4.3 (for SSE/Altivec?)
LFLAGS  = -lSaturn -levent -lcfitsio -lfftw3 -lcfitsio -lm 
SDLFLAGS = `sdl-config --libs --cflags`

CFLAGS	= $(MFLAGS) $(GFLAGS) $(OFLAGS) $(IFLAGS) $(LFLAGS) $(SDLFLAGS)

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



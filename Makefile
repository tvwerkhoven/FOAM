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
GFLAGS	= #-g -DDEBUG_ITIFG=255 -finstrument-functions # -pg for gprof profiling,  -finstrument-functions for Saturn profiling
IFLAGS  = -I. -L.
OFLAGS  =  -O3 -ftree-vectorize # tree-vectorize works with gcc 4.3 (for SSE/Altivec?)
LFLAGS  = -lSaturn -levent -lcfitsio -lfftw3 -lcfitsio -lm 
SDLFLAGS = `sdl-config --libs --cflags`

CFLAGS	= $(MFLAGS) $(GFLAGS) $(OFLAGS) $(IFLAGS) $(LFLAGS) $(SDLFLAGS)

# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
all:	foamcs-sim foamui

clean:	
	-rm -f foamcs-sim


# make multi-file foam
# ----------------------------------------------------------------------------

foamcs-sim: foam_cs.c foam_library.c foam_cs_library.c foam_modules-sim.c foam_modules-dm.c foam_modules-sh.c
		$(CC) $(CFLAGS) -lc $^ -o $@

foamui: foam_ui.c foam_library.c foam_ui_library.c
		$(CC) $(CFLAGS) -lc $^ -o $@



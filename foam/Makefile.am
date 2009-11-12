## @file foam/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

AM_CPPFLAGS = \
        -I$(top_srcdir)/lib \
        -I$(top_srcdir)/mods \
        -I$(top_srcdir)/$(LIBSIU)

# This is used to get the config directory as a define (see top-level Makefile.am)
# TODO: check this
AM_CPPFLAGS += -DFOAM_CONFDIR=\"$(FOAMCONFDIR)\" \
		-DFOAM_DATADIR=\"$(FOAMDATADIR)\" \
		$(COMMON_CFLAGS)

# GSL flags
AM_CPPFLAGS += -DHAVE_INLINE \
		-DGSL_RANGE_CHECK_OFF

if DEBUG
AM_CPPFLAGS += -DFOAM_DEBUG=1
endif DEBUG

AM_CFLAGS = -pipe -Wall -Wextra \
		-O3 -ftree-vectorize \
		-g

AM_LDFLAGS = $(top_builddir)/$(LIBSIU)/libsocket.a \
		$(top_builddir)/$(LIBSIU)/libio.a \
		$(top_builddir)/$(LIBSIU)/libprotocol.a \
		$(COMMON_LIBS)

# Basic framework files which are always needed
FRAME_SRC = foam.cc
FRAME_HDR = foam.h config.h

### DUMMY MODE ###
##################

bin_PROGRAMS = foam-dummy
foam_dummy_CFLAGS = -g $(AM_CFLAGS)
foam_dummy_CPPFLAGS = -DFOAM_DEBUG=1 $(AM_CPPFLAGS)
foam_dummy_SOURCES = foam-dummy.cc $(FRAME_SRC) 
foam_dummy_SOURCES += $(FRAME_HDR)

### END DUMMY MODE ###

### STATIC SIMULATION MODE ###
##############################

if SIMSTAT
bin_PROGRAMS += foam-simstat

foam_simstat_SOURCES = foam-simstatic.cc \
		$(FRAME_SRC) $(FRAME_HDR) \
		sh.c calib.c img.c log.c

# Are we using OpenGL or not?
if OPENGL
foam_simstat_SOURCES += dispgl.c
else !OPENGL
foam_simstat_SOURCES += dispsdl.c
endif !OPENGL

# header files are part of the sources as well
foam_simstat_SOURCES += sh.h calib.h dispcommon.h img.h foam-simstatic.h log.h

# Some CPP flags (include the primemod header, and configure some options)
foam_simstat_CPPFLAGS = -DFOAM_SIMSTAT_DISPLAY=1 -DFOAM_MODULES_DISLAY_SHSUPPORT=1 $(SIMSTAT_CFLAGS) $(AM_CPPFLAGS)

# specific flags and libraries
foam_simstat_CFLAGS = $(SIMSTAT_CFLAGS) $(AM_CFLAGS)
foam_simstat_LDFLAGS = $(SIMSTAT_LIBS) $(AM_LDFLAGS)
if OPENGL
foam_simstat_CPPFLAGS += -DFOAM_MODULES_DISPLAY_OPENGL=1 
# add some flags necessary for OpenGL
foam_simstat_CFLAGS += $(OPENGL_CFLAGS)
foam_simstat_LDFLAGS += $(OPENGL_LIBS)
endif OPENGL

endif SIMSTAT
### END STATIC SIMULATION MODE ###

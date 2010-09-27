## @file foam/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

MODS_DIR = $(top_builddir)/mods
LIB_DIR = $(top_builddir)/lib
LIBSIU_DIR = $(top_srcdir)/$(LIBSIU)

AM_CPPFLAGS = \
        -I$(LIB_DIR) \
        -I$(MODS_DIR) \
        -I$(LIBSIU_DIR)

# This is used to get the config directory as a define (see top-level Makefile.am)
# TODO: check this
AM_CPPFLAGS += -DFOAM_DATADIR=\"$(FOAMDATADIR)\" \
		$(COMMON_CFLAGS)

# GSL flags
AM_CPPFLAGS += -DHAVE_INLINE \
		-DGSL_RANGE_CHECK_OFF

if DEBUG
AM_CPPFLAGS += -DFOAM_DEBUG=1
endif DEBUG

AM_CXXFLAGS = -pipe -Wall -Wextra \
		-Wfatal-errors -Wno-unused-variable \
		-O3 -ftree-vectorize \
		-g

AM_LDFLAGS = $(COMMON_LIBS)

if HAVE_MACOSX
AM_LDFLAGS += -framework CoreFoundation -framework Carbon
endif

LDADD = $(LIBSIU_DIR)/libsocket.a \
		$(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libprotocol.a

# Basic framework files which are always needed
FRAME_SRC = foam.cc foamctrl.cc $(LIB_DIR)/devices.cc
FRAME_HDR = foam.h foamctrl.h config.h $(LIB_DIR)/devices.h

### DUMMY MODE ###
##################

bin_PROGRAMS = foam-dummy
foam_dummy_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(FOAMCONFDIR)/foam-dummy.cfg\" \
		-DFOAM_DEBUG=1 $(AM_CPPFLAGS)
foam_dummy_SOURCES = foam-dummy.cc $(FRAME_SRC) 
foam_dummy_SOURCES += $(FRAME_HDR)

### END DUMMY MODE ###

### STATIC SIMULATION MODE ###
##############################

if SIMSTAT
bin_PROGRAMS += foam-simstat

foam_simstat_SOURCES = foam-simstatic.cc \
		$(FRAME_SRC) $(FRAME_HDR) \
		$(LIB_DIR)/imgio.cc \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/dummycam.cc
#		$(MODS_DIR)/imgcam.cc

# Header files are part of the sources as well
foam_simstat_SOURCES += foam-simstatic.h \
		$(LIB_DIR)/imgio.h \
		$(MODS_DIR)/camera.h \
		$(MODS_DIR)/dummycam.h

# Some CPP flags (include the primemod header, and configure some options)
foam_simstat_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(FOAMCONFDIR)/foam-simstat.cfg\" \
		$(SIMSTAT_CFLAGS) $(AM_CPPFLAGS)

# specific flags and libraries
foam_simstat_CXXFLAGS = $(SIMSTAT_CFLAGS) $(AM_CXXFLAGS)
foam_simstat_LDFLAGS = $(SIMSTAT_LIBS) $(AM_LDFLAGS)

endif SIMSTAT
### END STATIC SIMULATION MODE ###

### HW TEST MODE ###
####################

if HWDEBUG
# building hardware tests

if HAVE_DC1394
bin_PROGRAMS += foam-hwtest

foam_hwtest_SOURCES = foam-hwtest.cc \
		$(FRAME_SRC) $(FRAME_HDR) \
		$(LIB_DIR)/imgio.cc \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/fw1394cam.cc

# Header files are part of the sources as well
foam_hwtest_SOURCES += foam-hwtest.h \
		$(LIB_DIR)/imgio.h \
		$(MODS_DIR)/camera.h \
		$(MODS_DIR)/fw1394cam.h \
		$(MODS_DIR)/dc1394++.h

foam_hwtest_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(FOAMCONFDIR)/foam-hwtest.cfg\" \
		$(AM_CPPFLAGS)
foam_hwtest_CXXFLAGS = $(AM_CXXFLAGS) $(DC1394_CFLAGS)
foam_hwtest_LDADD = $(LDADD) \
		$(DC1394_LIBS)

endif HAVE_DC1394

endif HWDEBUG
### END HW TEST MODE ###

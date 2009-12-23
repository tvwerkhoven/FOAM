## @file ui/Makefile.am
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
AM_CPPFLAGS += -DFOAM_CONFDIR=\"$(FOAMCONFDIR)\" \
		-DFOAM_DATADIR=\"$(FOAMDATADIR)\" \
		$(COMMON_CFLAGS)

if DEBUG
AM_CPPFLAGS += -DFOAM_DEBUG=1
endif DEBUG

AM_CXXFLAGS = -pipe -Wall -Wextra \
		-Wfatal-errors \
		-O3 -ftree-vectorize \
		-g

AM_LDFLAGS = $(COMMON_LIBS) $(GUI_LIBS)

LDADD = $(LIBSIU_DIR)/libsocket.a \
		$(LIBSIU_DIR)/libprotocol.a \
		-lgthread-2.0

### FOAM GUI ###
################

if GUI
bin_PROGRAMS = foam-gui
foam_gui_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
foam_gui_SOURCES = fgui.cc log.cc logview.cc about.cc foamcontrol.cc controlview.cc widgets.cc
foam_gui_SOURCES += fgui.h log.h logview.h about.h foamcontrol.h controlview.h widgets.h
endif GUI
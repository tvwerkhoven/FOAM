## @file ui/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

include $(top_srcdir)/common.mk

LDADD = $(LIBSIU_DIR)/libsocket.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libglviewer.a \
		$(LIBSIU_DIR)/libconfig.a \
		-lgthread-2.0

### FOAM GUI ###
################

if GUI
bin_PROGRAMS = foam-gui
foam_gui_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
foam_gui_LDFLAGS = $(COMMON_LIBS) $(GUI_LIBS) $(AM_LDFLAGS)
foam_gui_SOURCES = fgui.cc log.cc logview.cc about.cc foamcontrol.cc controlview.cc widgets.cc deviceview.cc devicectrl.cc camview.cc camctrl.cc wfsview.cc wfsctrl.cc shwfsview.cc shwfsctrl.cc
foam_gui_SOURCES += fgui.h log.h logview.h about.h foamcontrol.h controlview.h widgets.h deviceview.h devicectrl.h camview.h camctrl.h wfsview.h wfsctrl.h shwfsview.h shwfsctrl.h

foamdatadir = $(datadir)/foam
nobase_dist_foamdata_DATA = FOAM-logo.svg FOAM-logo.png

endif GUI
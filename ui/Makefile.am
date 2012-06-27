## @file ui/Makefile.am
## @author Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>

include $(top_srcdir)/common.mk

LDADD += $(LIBSIU_DIR)/libsocket.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libglviewer.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libsighandle.a \
		-lgthread-2.0

### FOAM GUI ###
################

if HAVE_GUI
bin_PROGRAMS = foam-gui
foam_gui_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
foam_gui_LDFLAGS = $(GUI_LIBS) $(AM_LDFLAGS)
foam_gui_SOURCES = fgui.cc log.cc logview.cc about.cc foamcontrol.cc controlview.cc widgets.cc deviceview.cc devicectrl.cc camview.cc camctrl.cc wfsview.cc wfsctrl.cc shwfsview.cc shwfsctrl.cc wfcview.cc wfcctrl.cc simcamview.cc simcamctrl.cc telescopeview.cc telescopectrl.cc
foam_gui_SOURCES += fgui.h log.h logview.h about.h foamcontrol.h controlview.h widgets.h deviceview.h devicectrl.h camview.h camctrl.h wfsview.h wfsctrl.h shwfsview.h shwfsctrl.h wfcview.h wfcctrl.h simcamview.h simcamctrl.h telescopeview.h telescopectrl.h

foamdatadir = $(datadir)/foam
nobase_dist_foamdata_DATA = FOAM-logo.svg FOAM-logo.png

endif HAVE_GUI
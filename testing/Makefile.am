## @file testing/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

MODS_DIR = $(top_builddir)/mods
LIB_DIR = $(top_builddir)/lib
LIBSIU_DIR = $(top_srcdir)/$(LIBSIU)

AM_CPPFLAGS = \
				-I$(LIB_DIR) \
				-I$(MODS_DIR) \
				-I$(LIBSIU_DIR) \
				$(COMMON_CFLAGS)

AM_CXXFLAGS = -pipe -Wall -Wextra \
		-Wfatal-errors -Wno-unused-variable \
		-O3 -ftree-vectorize \
		-g

AM_LDFLAGS = $(COMMON_LIBS) $(GUI_LIBS)

if HAVE_MACOSX
AM_LDFLAGS += -framework CoreFoundation -framework Carbon
endif

# foamtestdir = $(bindir)/foam-test
noinst_PROGRAMS = 

if HWDEBUG
# only do this if we want hardware debugging 
# install the test programs in a foam-test subdir
noinst_PROGRAMS += 
endif HWDEBUG

if DEBUG
noinst_PROGRAMS += pixbuf-test gtk-test dc1394-test

pixbuf_test_SOURCES = pixbuf-test.cc
pixbuf_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
pixbuf_test_LDADD = -lgthread-2.0

gtk_test_SOURCES = gtk-test.cc
gtk_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
gtk_test_LDADD = -lgthread-2.0

dc1394_test_SOURCES = dc1394-test.cc
dc1394_test_CXXFLAGS = $(AM_CXXFLAGS) $(DC1394_CFLAGS)
dc1394_test_LDADD = $(LDADD) $(DC1394_LIBS)

### OpenGL programs go here (need GUI libs)

if HAVE_OPENGL

noinst_PROGRAMS += opengl-test camview-test 

opengl_test_SOURCES = opengl-test.cc
opengl_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
opengl_test_CFLAGS = $(GUI_CFLAGS)
opengl_test_LDADD = $(GUI_LIBS) $(LIBSIU_DIR)/libglviewer.a -lgthread-2.0

camview_test_SOURCES = camview-test.cc
camview_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
camview_test_CFLAGS = $(GUI_CFLAGS) 
camview_test_LDADD = $(GUI_CFLAGS) -lgthread-2.0 \
		$(LIBSIU_DIR)/libglviewer.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libsocket.a

endif HAVE_OPENGL

if HAVE_FITSIO

noinst_PROGRAMS += camsend-test

camsend_test_SOURCES = camsend-test.cc
camsend_test_LDADD = $(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libsocket.a

endif HAVE_FITSIO

endif DEBUG
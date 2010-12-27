## @file testing/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

include $(top_srcdir)/common.mk

AM_LDFLAGS = $(COMMON_LIBS) $(GUI_LIBS)

if HAVE_MACOSX
AM_LDFLAGS += -framework CoreFoundation -framework Carbon
endif

noinst_PROGRAMS = autotools-test

autotools_test_SOURCES = autotools-test.cc

# include compilation test
TESTS_ENVIRONMENT = $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(autotools_test_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -E -dM
TESTS = $(autotools_test_SOURCES)
		

if HWDEBUG
# only do this if we want hardware debugging 
# install the test programs in a foam-test subdir
noinst_PROGRAMS += 
if HAVE_DC1394
noinst_PROGRAMS += dc1394-test

dc1394_test_SOURCES = dc1394-test.cc
dc1394_test_CXXFLAGS = $(AM_CXXFLAGS) $(DC1394_CFLAGS)
dc1394_test_LDADD = $(LDADD) $(DC1394_LIBS)
endif
endif HWDEBUG

if DEBUG
noinst_PROGRAMS += pixbuf-test gtk-test sigcpp-test

pixbuf_test_SOURCES = pixbuf-test.cc
pixbuf_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
pixbuf_test_LDADD = -lgthread-2.0

gtk_test_SOURCES = gtk-test.cc
gtk_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
gtk_test_LDADD = -lgthread-2.0

sigcpp_test_SOURCES = sigcpp-test.cc
sigcpp_test_CPPFLAGS = $(SIGC_CFLAGS) $(AM_CPPFLAGS)
sigcpp_test_LDADD = $(LDADD) $(SIGC_LIBS)

if FULLSIM
noinst_PROGRAMS += shwfs-test
shwfs_test_SOURCES = shwfs-test.cc \
		$(MODS_DIR)/shwfs.cc \
		$(MODS_DIR)/wfs.cc \
		$(MODS_DIR)/dummycam.cc \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/simulcam.cc \
		$(LIB_DIR)/devices.cc \
		$(LIB_DIR)/simseeing.cc \
		../foam/foamctrl.cc
shwfs_test_CFLAGS = $(FFTW_CFLAGS)
shwfs_test_CXXFLAGS = $(AM_CXXFLAGS)
shwfs_test_LDADD = $(FFTW_LIBS) \
		$(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libpath.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libimgdata.a
endif FULLSIM

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

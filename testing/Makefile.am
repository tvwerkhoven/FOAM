## @file testing/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)


MODS_DIR = $(top_builddir)/mods
LIB_DIR = $(top_builddir)/lib
LIBSIU_DIR = $(top_srcdir)/$(LIBSIU)

AM_CPPFLAGS = -D__STDC_FORMAT_MACROS \
		-D__STDC_LIMIT_MACROS \
		-DFOAM_DATADIR=\"$(FOAMDATADIR)\"

AM_CPPFLAGS += \
        -I$(LIB_DIR) \
        -I$(MODS_DIR) \
        -I$(LIBSIU_DIR)
								
#AM_LDFLAGS =

if HAVE_MACOSX
#AM_LDFLAGS += -framework CoreFoundation -framework Carbon
endif

## Compilation tests
check_PROGRAMS = autotools-test

autotools_test_SOURCES = autotools-test.cc

# include compilation test
TESTS_ENVIRONMENT = $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(autotools_test_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -E -dM
TESTS = $(autotools_test_SOURCES)

## Bitness test
#check_PROGRAMS += bit64-test bit32-test bitdef-test
check_PROGRAMS += bitdef-test

bitdef_test_SOURCES = bitness-test.cc
bitdef_test_LDFLAGS = 
bitdef_test_LIBS = 
#bit64_test_SOURCES = $(bitdef_test_SOURCES)
#bit64_test_CXXFLAGS = -m64 $(AM_CXXFLAGS)
#bit64_test_LDFLAGS = 
#bit64_test_LIBS = 
#bit32_test_SOURCES = $(bitdef_test_SOURCES)
#bit32_test_CXXFLAGS = -m32 $(AM_CXXFLAGS)
#bit32_test_LDFLAGS = 
#bit32_test_LIBS =

## Latencies test
check_PROGRAMS += latencies-test

latencies_test_SOURCES = latencies-test.cc
latencies_test_CPPFLAGS = $(SIGC_CFLAGS) $(AM_CPPFLAGS)
latencies_test_LDADD = $(LIBSIU_DIR)/libtime.a \
		$(LDADD) $(SIGC_LIBS)

if HWDEBUG
# only do this if we want hardware debugging 
# install the test programs in a foam-test subdir
if HAVE_DC1394
check_PROGRAMS += dc1394-test

dc1394_test_SOURCES = dc1394-test.cc
dc1394_test_CPPFLAGS = $(DC1394_CFLAGS) $(AM_CPPFLAGS)
dc1394_test_LDADD = $(LDADD) $(DC1394_LIBS)
endif HAVE_DC1394

if HAVE_ANDORCAM
check_PROGRAMS += andorcam-test

andorcam_test_SOURCES = andorcam-test.cc \
		$(MODS_DIR)/andor.cc \
		$(MODS_DIR)/camera.cc \
		$(LIB_DIR)/devices.cc \
		../foam/foamctrl.cc

andorcam_test_CPPFLAGS = $(SIGC_CFLAGS) $(AM_CPPFLAGS)
andorcam_test_LDADD = $(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libpath.a \
		$(LDADD) $(SIGC_LIBS) $(ANDORCAM_LIBS)

endif HAVE_ANDORCAM

endif HWDEBUG

if DEBUG
check_PROGRAMS += pixbuf-test gtk-test sigcpp-test zern-test

pixbuf_test_SOURCES = pixbuf-test.cc
pixbuf_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
pixbuf_test_LDADD = $(GUI_LIBS) -lgthread-2.0

gtk_test_SOURCES = gtk-test.cc
gtk_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
gtk_test_LDADD = $(GUI_LIBS) -lgthread-2.0

sigcpp_test_SOURCES = sigcpp-test.cc
sigcpp_test_CPPFLAGS = $(SIGC_CFLAGS) $(AM_CPPFLAGS)
sigcpp_test_LDADD = $(LDADD) $(SIGC_LIBS)

zern_test_SOURCES = zern-test.cc \
		$(LIB_DIR)/zernike.cc
zern_test_CPPFLAGS = $(COMMON_CFLAGS) $(AM_CPPFLAGS)
zern_test_LDADD = $(LIBSIU_DIR)/libimgdata.a \
		$(LIBSIU_DIR)/libio.a \
		$(COMMON_LIBS)


if FULLSIM
check_PROGRAMS += shwfs-test
shwfs_test_SOURCES = shwfs-test.cc \
		$(MODS_DIR)/shwfs.cc \
		$(MODS_DIR)/wfs.cc \
    $(MODS_DIR)/wfc.cc \
		$(MODS_DIR)/dummycam.cc \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/simulcam.cc \
    $(MODS_DIR)/simulwfc.cc \
		$(LIB_DIR)/shift.cc \
		$(LIB_DIR)/devices.cc \
		$(LIB_DIR)/simseeing.cc \
		$(LIB_DIR)/zernike.cc \
		../foam/foamctrl.cc

shwfs_test_CFLAGS = $(COMMON_CFLAGS) $(FFTW_CFLAGS)
shwfs_test_CPPFLAGS = $(SIGC_CFLAGS) $(AM_CPPFLAGS)
shwfs_test_LDADD = $(COMMON_LIBS) \
		$(SIGC_LIBS) \
		$(FFTW_LIBS) \
    $(LIBSIU_DIR)/libcsv.a \
		$(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libpath.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libimgdata.a

endif FULLSIM

### OpenGL programs go here (need GUI libs)

if HAVE_OPENGL

check_PROGRAMS += camview-test 

camview_test_SOURCES = camview-test.cc
camview_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
camview_test_CFLAGS = $(COMMON_CFLAGS) $(GUI_CFLAGS) 
camview_test_LDADD = $(COMMON_LIBS) $(GUI_LIBS) -lgthread-2.0 \
		$(LIBSIU_DIR)/libglviewer.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libsocket.a

endif HAVE_OPENGL

check_PROGRAMS += camsend-test

camsend_test_SOURCES = camsend-test.cc
camsend_test_CPPFLAGS = $(COMMON_CFLAGS) $(AM_CPPFLAGS)
camsend_test_LDADD = $(COMMON_LIBS) \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libsocket.a


endif DEBUG

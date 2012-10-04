## @file testing/Makefile.am
## @author Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>

include $(top_srcdir)/common.mk

if HAVE_MACOSX
#AM_LDFLAGS += -framework CoreFoundation -framework Carbon
endif

##############################################################################
# MISCELLANEOUS TEST PROGRAMS
##############################################################################

## Compilation tests
check_PROGRAMS = autotools-test

autotools_test_SOURCES = autotools-test.cc

# include compilation test
#TESTS_ENVIRONMENT = $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
#	$(autotools_test_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -E -dM
#TESTS = $(autotools_test_SOURCES)

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
latencies_test_LDADD = $(LIBSIU_DIR)/libtime.a \
		$(LDADD)


check_PROGRAMS += camsend-test pixbuf-test gtk-test sigcpp-test 

camsend_test_SOURCES = camsend-test.cc
camsend_test_LDADD = $(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libsocket.a \
		$(LDADD)


pixbuf_test_SOURCES = pixbuf-test.cc
pixbuf_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
pixbuf_test_LDADD = $(GUI_LIBS) -lgthread-2.0 $(LDADD)

gtk_test_SOURCES = gtk-test.cc
gtk_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
gtk_test_LDADD = $(GUI_LIBS) -lgthread-2.0 $(LDADD)

sigcpp_test_SOURCES = sigcpp-test.cc

if HAVE_FULLSIM
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
		$(FOAM_DIR)/foamctrl.cc

shwfs_test_CFLAGS = $(COMMON_CFLAGS) $(FFTW_CFLAGS) $(AM_CFLAGS)
shwfs_test_LDADD = $(LIBSIU_DIR)/libcsv.a \
		$(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libpath.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libimgdata.a \
		$(FFTW_LIBS) \
		$(LDADD)

endif HAVE_FULLSIM

### OpenGL programs go here (need GUI libs)

if HAVE_OPENGL

check_PROGRAMS += camview-test 

camview_test_SOURCES = camview-test.cc
camview_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
camview_test_CXXFLAGS = $(GUI_CFLAGS) $(AM_CXXFLAGS)
camview_test_LDADD = $(LIBSIU_DIR)/libglviewer.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libsocket.a \
		$(GUI_LIBS) -lgthread-2.0 \
		$(LDADD)

endif HAVE_OPENGL

##############################################################################
# HARDWARE TEST PROGRAMS
##############################################################################

if HAVE_HWDEBUG
# only do this if we want hardware debugging 
# install the test programs in a foam-test subdir

### IEEE1394 (Firewire) camera test

if HAVE_DC1394
check_PROGRAMS += dc1394-test

dc1394_test_SOURCES = dc1394-test.cc
dc1394_test_CPPFLAGS = $(DC1394_CFLAGS) $(AM_CPPFLAGS)
dc1394_test_CXXFLAGS = $(DC1394_CFLAGS) $(AM_CXXFLAGS)
dc1394_test_LDADD = $(DC1394_LIBS) $(LDADD)
endif HAVE_DC1394

### Andor iXon EMCCD test

if HAVE_ANDORCAM
check_PROGRAMS += andorcam-test

andorcam_test_SOURCES = andorcam-test.cc \
		$(MODS_DIR)/andor.cc \
		$(MODS_DIR)/camera.cc \
		$(LIB_DIR)/devices.cc \
		$(FOAM_DIR)/foamctrl.cc

andorcam_test_LDADD = $(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libpath.a \
		$(ANDORCAM_LIBS) $(LDADD)

endif HAVE_ANDORCAM

### Alpao deformable mirror test 

if HAVE_ALPAODM

check_PROGRAMS += alpaodm-test

alpaodm_test_SOURCES = alpaodm-test.cc \
		$(MODS_DIR)/alpaodm.cc \
		$(MODS_DIR)/wfc.cc \
		$(LIB_DIR)/devices.cc \
		$(FOAM_DIR)/foamctrl.cc

alpaodm_test_LDADD = $(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libpath.a \
		$(ALPAODM_LIBS) $(LDADD) 

endif HAVE_ALPAODM

endif HAVE_HWDEBUG

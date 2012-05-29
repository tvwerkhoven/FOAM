## @file foam/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

include $(top_srcdir)/common.mk

if HAVE_MACOSX
AM_LDFLAGS = -framework CoreFoundation -framework Carbon
endif

LDADD += $(LIBSIU_DIR)/libsocket.a \
		$(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libperflogger.a \
		$(LIBSIU_DIR)/libpath.a \
		$(LIBSIU_DIR)/libsighandle.a
		

# Basic framework files which are always needed
FRAME_SRC = foam.cc foamctrl.cc $(LIB_DIR)/devices.cc
FRAME_HDR = foam.h foamctrl.h autoconfig.h $(LIB_DIR)/devices.h

# init empty, append later
bin_PROGRAMS =

### DUMMY MODE ###
##################

if HAVE_DUMMY

bin_PROGRAMS += foam-dummy
foam_dummy_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(sysconfdir)/foam/foam-dummy.cfg\" \
		$(AM_CPPFLAGS)
foam_dummy_SOURCES = foam-dummy.cc $(FRAME_SRC) 
foam_dummy_SOURCES += $(FRAME_HDR)

endif HAVE_DUMMY
### END DUMMY MODE ###

### STATIC SIMULATION MODE ###
##############################

if HAVE_SIMSTAT
bin_PROGRAMS += foam-simstat

foam_simstat_SOURCES = foam-simstatic.cc \
		$(FRAME_SRC) \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/dummycam.cc \
		$(MODS_DIR)/imgcam.cc \
		$(MODS_DIR)/wfc.cc \
		$(MODS_DIR)/wfs.cc \
		$(MODS_DIR)/shwfs.cc \
		$(LIB_DIR)/shift.cc

# Header files are part of the sources as well
foam_simstat_SOURCES += foam-simstatic.h \
		$(FRAME_HDR) \
		$(MODS_DIR)/camera.h \
		$(MODS_DIR)/dummycam.h \
		$(MODS_DIR)/imgcam.h \
		$(MODS_DIR)/wfc.h \
		$(MODS_DIR)/wfs.h \
		$(MODS_DIR)/shwfs.h \
		$(LIB_DIR)/shift.h

# Some CPP flags
foam_simstat_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(sysconfdir)/foam/foam-simstat.cfg\" \
		$(SIMSTAT_CFLAGS) $(AM_CPPFLAGS)

foam_simstat_LDADD = $(LIBSIU_DIR)/libimgdata.a \
		$(LIBSIU_DIR)/libcsv.a \
		$(LDADD)

# specific flags and libraries
foam_simstat_CXXFLAGS = $(SIMSTAT_CFLAGS) $(AM_CXXFLAGS)
foam_simstat_LDFLAGS = $(SIMSTAT_LIBS) $(AM_LDFLAGS)

endif HAVE_SIMSTAT
### END STATIC SIMULATION MODE ###

### HW TEST MODE ###
####################

if HAVE_HWDEBUG
# building hardware tests

if HAVE_DC1394
bin_PROGRAMS += foam-hwtest

foam_hwtest_SOURCES = foam-hwtest.cc \
		$(FRAME_SRC) \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/fw1394cam.cc

# Header files are part of the sources as well
foam_hwtest_SOURCES += foam-hwtest.h \
		$(FRAME_HDR) \
		$(MODS_DIR)/camera.h \
		$(MODS_DIR)/fw1394cam.h \
		$(MODS_DIR)/dc1394++.h

foam_hwtest_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(sysconfdir)/foam/foam-hwtest.cfg\" \
		$(AM_CPPFLAGS)
foam_hwtest_CXXFLAGS = $(AM_CXXFLAGS) $(DC1394_CFLAGS)
foam_hwtest_LDADD = $(LDADD) \
		$(DC1394_LIBS)

endif HAVE_DC1394

endif HAVE_HWDEBUG
### END HW TEST MODE ###


### FULL SIMULATION MODE ###
############################

if HAVE_FULLSIM
# building full simulation mode

bin_PROGRAMS += foam-fullsim

foam_fullsim_SOURCES = foam-fullsim.cc \
		$(FRAME_SRC) \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/simulcam.cc \
		$(MODS_DIR)/shwfs.cc \
		$(MODS_DIR)/wfs.cc \
		$(MODS_DIR)/simulwfc.cc \
		$(MODS_DIR)/wfc.cc \
		$(LIB_DIR)/shift.cc \
		$(LIB_DIR)/zernike.cc \
		$(LIB_DIR)/simseeing.cc

# Header files are part of the sources as well
foam_fullsim_SOURCES += foam-fullsim.h \
		$(FRAME_HDR) \
		$(MODS_DIR)/camera.h \
		$(MODS_DIR)/simulcam.h \
		$(MODS_DIR)/shwfs.h \
		$(MODS_DIR)/wfs.h \
		$(MODS_DIR)/simulwfc.h \
		$(MODS_DIR)/wfc.h \
		$(LIB_DIR)/shift.h \
		$(LIB_DIR)/zernike.h \
		$(LIB_DIR)/simseeing.h

foam_fullsim_LDADD = $(LIBSIU_DIR)/libimgdata.a \
		$(LIBSIU_DIR)/libcsv.a \
		$(LDADD)

# Some CPP flags
foam_fullsim_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(sysconfdir)/foam/foam-fullsim.cfg\" \
		$(FULLSIM_CFLAGS) $(AM_CPPFLAGS)

# specific flags and libraries
foam_fullsim_CXXFLAGS = $(FULLSIM_CFLAGS) $(AM_CXXFLAGS)
foam_fullsim_LDFLAGS = $(FULLSIM_LIBS) $(AM_LDFLAGS)

endif HAVE_FULLSIM
### END FULL SIMULATION MODE ###

### EXPOAO BUILD TARGET ###
###########################

if HAVE_EXPOAO
# building expoao foam target

bin_PROGRAMS += foam-expoao

foam_expoao_SOURCES = foam-expoao.cc \
		$(FRAME_SRC) \
		$(MODS_DIR)/camera.cc \
		$(MODS_DIR)/andor.cc \
		$(MODS_DIR)/shwfs.cc \
		$(MODS_DIR)/wfs.cc \
		$(MODS_DIR)/wfc.cc \
		$(MODS_DIR)/alpaodm.cc \
		$(MODS_DIR)/telescope.cc \
		$(MODS_DIR)/wht.cc \
		$(LIB_DIR)/shift.cc

# Header files are part of the sources as well
foam_expoao_SOURCES += foam-expoao.h \
		$(FRAME_HDR) \
		$(MODS_DIR)/camera.h \
		$(MODS_DIR)/andor.h \
		$(MODS_DIR)/shwfs.h \
		$(MODS_DIR)/wfs.h \
		$(MODS_DIR)/wfc.h \
		$(MODS_DIR)/alpaodm.h \
		$(MODS_DIR)/telescope.h \
		$(MODS_DIR)/wht.h \
		$(LIB_DIR)/shift.h

foam_expoao_CPPFLAGS = -DFOAM_DEFAULTCONF=\"$(sysconfdir)/foam/foam-expoao.cfg\" \
		$(EXPOAO_CFLAGS) $(AM_CPPFLAGS)

foam_expoao_CXXFLAGS = $(EXPOAO_CFLAGS) $(AM_CXXFLAGS)

foam_expoao_LDADD = $(EXPOAO_LIBS) $(LDADD)

endif HAVE_EXPOAO
### END EXPOAO TARGET ###


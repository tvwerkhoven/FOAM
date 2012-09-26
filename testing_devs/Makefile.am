## @file testing_devs/Makefile.am
## @author Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>

include $(top_srcdir)/common.mk

if HAVE_MACOSX
#AM_LDFLAGS += -framework CoreFoundation -framework Carbon
endif

# Files to include to add a FOAM_dummy class
FOAMDUMMY_SRC = $(FOAM_DIR)/foam.cc $(FOAM_DIR)/foamctrl.cc 
FOAMDUMMY_HDR = $(FOAM_DIR)/foam.h $(FOAM_DIR)/foamctrl.h $(FOAM_DIR)/autoconfig.h
FOAMDUMMY_LDADD = $(LIBSIU_DIR)/libsocket.a \
		$(LIBSIU_DIR)/libio.a \
		$(LIBSIU_DIR)/libconfig.a \
		$(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libperflogger.a \
		$(LIBSIU_DIR)/libpath.a \
		$(LIBSIU_DIR)/libsighandle.a


##############################################################################
# HARDWARE TEST PROGRAMS
##############################################################################

if HAVE_HWDEBUG
# only do this if we want hardware debugging 
# install the test programs in a foam-test subdir

### WHT telescope guiding test

check_PROGRAMS = wht-test

wht_test_SOURCES = wht-test.cc \
		$(FOAMDUMMY_SRC) \
		$(FOAMDUMMY_HDR) \
		$(MODS_DIR)/telescope.cc \
		$(MODS_DIR)/wht.cc \
		$(LIB_DIR)/devices.cc

wht_test_LDADD = $(LIBSIU_DIR)/libserial.a \
		$(FOAMDUMMY_LDADD) \
		$(LDADD)

endif HAVE_HWDEBUG

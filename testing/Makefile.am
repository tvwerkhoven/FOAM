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

# foamtestdir = $(bindir)/foam-test
noinst_PROGRAMS = 

if HWDEBUG
# only do this if we want hardware debugging 
# install the test programs in a foam-test subdir
noinst_PROGRAMS += 
endif HWDEBUG

if DEBUG
noinst_PROGRAMS += imgio-test imgio-convert-test io-test protocol-test pixbuf-test 

io_test_SOURCES = io-test.cc
io_test_LDADD = $(LIBSIU_DIR)/libio.a

imgio_test_SOURCES = imgio-test.cc \
		$(LIB_DIR)/imgio.cc $(LIB_DIR)/imgio.h
imgio_test_LDADD = $(LIBSIU_DIR)/libio.a

imgio_convert_test_SOURCES = imgio-convert-test.cc \
		$(LIB_DIR)/imgio.cc $(LIB_DIR)/imgio.h
imgio_convert_test_LDADD = $(LIBSIU_DIR)/libio.a

protocol_test_SOURCES = protocol-test.cc
protocol_test_LDADD = $(LIBSIU_DIR)/libprotocol.a \
		$(LIBSIU_DIR)/libsocket.a

pixbuf_test_SOURCES = pixbuf-test.cc
pixbuf_test_CPPFLAGS = $(GUI_CFLAGS) $(AM_CPPFLAGS)
pixbuf_test_LDADD = -lgthread-2.0

if OPENGL

noinst_PROGRAMS += opengl-test

opengl_test_SOURCES = opengl-test.cc
opengl_test_CPPFLAGS = $(GUI_CFLAGS) $(OPENGL_CFLAGS) $(AM_CPPFLAGS)
opengl_test_CFLAGS = $(GUI_CFLAGS) $(OPENGL_CFLAGS) 
opengl_test_LDADD = $(OPENGL_LIBS) $(LIBSIU_DIR)/libglviewer.a -lgthread-2.0

endif OPENGL
endif DEBUG
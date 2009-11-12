## @file lib/Makefile.am
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

AM_CPPFLAGS = -I$(top_srcdir)/$(LIBSIU) $(COMMON_CFLAGS)

noinst_LIBRARIES = liblog.a
# libsocket.a libio.a libprotocol.a

liblog_a_SOURCES = log.cc
#liblog_a_LIBADD = $(COMMON_LIBS)
#liblog_a_CPPFLAGS = -I$(top_srcdir)/$(LIBSIU) $(COMMON_CFLAGS)
#libsocket_a_SOURCES = socket.cc
#libio_a_SOURCES = io.cc
#libprotocol_a_SOURCES = protocol.cc
#libprotocol_a_LIBADD =  $(SIGCPP_LIBS)
#libprotocol_a_CPPFLAGS = $(COMMON_CFLAGS)

noinst_HEADERS = log.h 
#socket.h format.h io.h protocol.h
noinst_LIBRARIES = liblog.a libsocket.a libio.a libprotocol.a

liblog_a_SOURCES = log.c 
libsocket_a_SOURCES = socket.cc
libio_a_SOURCES = io.cc
libprotocol_a_SOURCES = protocol.cc
#libprotocol_a_LIBADD =  $(SIGCPP_LIBS)
libprotocol_a_CPPFLAGS = $(SIGCPP_CFLAGS)

noinst_HEADERS = log.h socket.h format.h io.h protocol.h
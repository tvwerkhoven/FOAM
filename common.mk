## @file common.mk
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
## Common makefile directives for all subdirs.

### Inclusion directories

MODS_DIR = $(top_builddir)/mods
LIB_DIR = $(top_builddir)/lib
LIBSIU_DIR = $(top_srcdir)/$(LIBSIU)

AM_CPPFLAGS = \
        -I$(LIB_DIR) \
        -I$(MODS_DIR) \
        -I$(LIBSIU_DIR) \
				$(COMMON_CFLAGS)

### Common flags
AM_CPPFLAGS += -D__STDC_FORMAT_MACROS \
		-D__STDC_LIMIT_MACROS \
		-DFOAM_DATADIR=\"$(datadir)/foam\"

### GSL flags
AM_CPPFLAGS += -DHAVE_INLINE 

if !HAVE_DEBUG
AM_CPPFLAGS += -DGSL_RANGE_CHECK_OFF
endif !HAVE_DEBUG

### More error reporting during compilation
AM_CXXFLAGS = -Wall -Wextra -Wfatal-errors

### Debug flags
if HAVE_DEBUG
AM_CPPFLAGS += -D_GLIBCXX_DEBUG \
		-D_GLIBCXX_DEBUG_PEDANTIC \
		-DFOAM_DEBUG=1
AM_CXXFLAGS += -O1 -ggdb -fno-inline
else
AM_CXXFLAGS += -O3 -ftree-vectorize
endif
		
	
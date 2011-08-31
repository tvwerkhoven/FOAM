## @file common.mk
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
## Common makefile directives for all subdirs.

### Inclusion directories

MODS_DIR = $(top_builddir)/mods
LIB_DIR = $(top_builddir)/lib
LIBSIU_DIR = $(top_srcdir)/$(LIBSIU)

### Common flags

AM_CPPFLAGS = \
        -I$(LIB_DIR) \
        -I$(MODS_DIR) \
        -I$(LIBSIU_DIR) \
				$(COMMON_CFLAGS)

AM_CPPFLAGS += -D__STDC_FORMAT_MACROS \
		-D__STDC_LIMIT_MACROS \
		-DFOAM_DATADIR=\"$(datadir)/foam\"

LDADD = $(COMMON_LIBS)

# More error reporting during compilation
AM_CXXFLAGS = -Wall -Wextra -Wfatal-errors

### GSL flags
AM_CPPFLAGS += -DHAVE_INLINE 

if !HAVE_DEBUG
AM_CPPFLAGS += -DGSL_RANGE_CHECK_OFF
endif !HAVE_DEBUG

### Debug options (first check strict debug, then regular. Add flags accordingly)
if HAVE_STR_DEBUG
AM_CPPFLAGS += -D_GLIBCXX_DEBUG \
		-D_GLIBCXX_DEBUG_PEDANTIC \
		-D_GLIBCXX_FULLY_DYNAMIC_STRING \
		-DGLIBCXX_FORCE_NEW
endif

if HAVE_DEBUG
AM_CXXFLAGS += -ggdb -g3 -O0 -fno-inline
else
AM_CXXFLAGS += -O3 -ftree-vectorize
endif

### Profiling options
if HAVE_PROFILING
AM_CXXFLAGS += -pg
endif HAVE_PROFILING
	
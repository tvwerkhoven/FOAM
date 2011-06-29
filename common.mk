## @file common.mk
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
## Common makefile directives for all subdirs.

MODS_DIR = $(top_builddir)/mods
LIB_DIR = $(top_builddir)/lib
LIBSIU_DIR = $(top_srcdir)/$(LIBSIU)

AM_CPPFLAGS = -D__STDC_FORMAT_MACROS \
		-D__STDC_LIMIT_MACROS \
		-DFOAM_DATADIR=\"$(FOAMDATADIR)\"

AM_CPPFLAGS += \
        -I$(LIB_DIR) \
        -I$(MODS_DIR) \
        -I$(LIBSIU_DIR) \
				$(COMMON_CFLAGS)

# GSL flags
AM_CPPFLAGS += -DHAVE_INLINE 
#\
#		-DGSL_RANGE_CHECK_OFF


#if DEBUG
#AM_CPPFLAGS += -DFOAM_DEBUG=1
#endif DEBUG

# Debug flags
AM_CXXFLAGS = -Wall -Wextra -Wfatal-errors -O1 -ggdb -fno-inline
# Speed flags
#AM_CXXFLAGS = -Wall -Wextra -Wfatal-errors -O3 -ftree-vectorize
		
	
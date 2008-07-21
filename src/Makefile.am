# init the bin_programs variable
bin_PROGRAMS = 

# some shared compiler flags which must always be used
# (not using AM_* flags because of http://sourceware.org/automake/automake.html#Flag-Variables-Ordering)
#CC=/usr/bin/gcc-4.1
CFLAGS=-pipe -Wall -Wextra 
# To diagnose linker errors:
#CFLAGS+=-Wl,-verbose
# To get some optimization:
#CFLAGS+=-O3 -ftree-vectorize

LDFLAGS=$(COMMON_LIBS) -lc

# This is used to get the config directory as a define (see top-level Makefile.am)
CPPFLAGS+=-DFOAM_CONFDIR=\"$(FOAMCONFDIR)\" -DFOAM_DATADIR=\"$(FOAMDATADIR)\" 
# (un)comment this to toggle GSL inlining
CPPFLAGS+=-DHAVE_INLINE -DGSL_RANGE_CHECK_OFF 
# (un) comment this to toggle lowlevel debugging in some modules 
# absolutely not recommended for production systems
CPPFLAGS+=-DFOAM_DEBUG=1
# This is necessary for itifg
CPPFLAGS+= -DDEBUG_ITIFG=255 -DLinux

# (un)comment this to toggle debugging
CFLAGS+=-g
# (un)comment these to toggle malloc debugging
#HAVE_DMALLOC=1
#LDFLAGS+=-ldmalloc 

# basic framework files which are always needed
FRAME_SRC = foam_cs.c foam_cs_library.c
FRAME_HDR = foam_cs_library.h foam_cs_config.h

# dummy moduleset
bin_PROGRAMS += foamcs-dummy
foamcs_dummy_SOURCES = foam_primemod-dummy.c $(FRAME_SRC) 
foamcs_dummy_SOURCES += foam_primemod-dummy.h $(FRAME_HDR)
foamcs_dummy_CPPFLAGS = -include "foam_primemod-dummy.h"
foamcs_dummy_CFLAGS = -g

### UI MODE ###
###############
if UI
bin_PROGRAMS += foamui
foamui_SOURCES = foam_ui.c foam_ui_library.c
foamui_SOURCES += foam_ui_library.h
foamui_CFLAGS = $(UI_CFLAGS)
foamui_LDFLAGS = $(UI_LIBS)
endif UI
### END UI MODE ###

### STATIC SIMULATION MODE ###
##############################

if SIMSTAT
# Program name simstat
bin_PROGRAMS += foamcs-simstat

# prime module C file
foamcs_simstat_SOURCES = foam_primemod-simstatic.c
# Other C files
foamcs_simstat_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_simstat_SOURCES += foam_modules-sh.c foam_modules-calib.c foam_modules-img.c foam_modules-log.c

# Are we going to use OpenGL or not?
if OPENGL
foamcs_simstat_SOURCES += foam_modules-dispgl.c
else !OPENGL
foamcs_simstat_SOURCES += foam_modules-dispsdl.c
endif !OPENGL

# header files are part of the sources as well
foamcs_simstat_SOURCES += foam_modules-sh.h foam_modules-calib.h foam_modules-dispcommon.h foam_modules-img.h foam_primemod-simstatic.h foam_modules-log.h

# Some CPP flags (include the primemod header, and configure some options)
foamcs_simstat_CPPFLAGS = -include "foam_primemod-simstatic.h"
#foamcs_mcmath_CPPFLAGS += -DDEBUG_ITIFG=255 
foamcs_simstat_CPPFLAGS += -DFOAM_SIMSTAT_DISPLAY=1 -DFOAM_MODULES_DISLAY_SHSUPPORT=1

# specific flags and libraries
foamcs_simstat_CFLAGS = $(SIMSTAT_CFLAGS)
foamcs_simstat_LDFLAGS = $(SIMSTAT_LIBS)
foamcs_simstat_CPPFLAGS += -DFOAM_MODULES_DISPLAY_OPENGL=1 
if OPENGL
# add some flags necessary for OpenGL
foamcs_simstat_CFLAGS += $(OPENGL_CFLAGS)
foamcs_simstat_LDFLAGS += $(OPENGL_LIBS)
endif OPENGL

endif SIMSTAT
### END STATIC SIMULATION MODE ###

### DYNAMICAL SIMULATION MODE ###
#################################

if SIMDYN
# Program name simstat
bin_PROGRAMS += foamcs-simdyn

# prime module C file
foamcs_simdyn_SOURCES = foam_primemod-simdyn.c
# Other C files
foamcs_simdyn_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_simdyn_SOURCES += foam_modules-sh.c foam_modules-calib.c foam_modules-img.c foam_modules-sim.c foam_modules-log.c

# Are we going to use OpenGL or not?
if OPENGL
foamcs_simdyn_SOURCES += foam_modules-dispgl.c
else !OPENGL
foamcs_simdyn_SOURCES += foam_modules-dispsdl.c
endif !OPENGL

# header files are part of the sources as well
foamcs_simdyn_SOURCES += foam_modules-sh.h foam_modules-calib.h foam_modules-dispcommon.h foam_modules-img.h foam_modules-sim.h foam_modules-log.h foam_primemod-simdyn.h

# Some CPP flags (include the primemod header, and configure some options)
foamcs_simdyn_CPPFLAGS = -include "foam_primemod-simdyn.h"
#foamcs_mcmath_CPPFLAGS += -DDEBUG_ITIFG=255 
foamcs_simdyn_CPPFLAGS += -DFOAM_SIMDYN_DISPLAY=1 -DFOAM_MODULES_DISLAY_SHSUPPORT=1

# specific flags and libraries
foamcs_simdyn_CFLAGS = $(SIMDYN_CFLAGS)
foamcs_simdyn_LDFLAGS = $(SIMDYN_LIBS)
foamcs_simdyn_CPPFLAGS += -DFOAM_MODULES_DISPLAY_OPENGL=1 
if OPENGL
# add some flags necessary for OpenGL
foamcs_simdyn_CFLAGS += $(OPENGL_CFLAGS)
foamcs_simdyn_LDFLAGS += $(OPENGL_LIBS)
endif OPENGL

endif SIMDYN
### END DYNAMICAL SIMULATION MODE ###

### MCMATH MODE ### 
###################
if MCMATH
# Compile mcmath site specific prime module
bin_PROGRAMS += foamcs-mcmath

# Use the prime module C-file
foamcs_mcmath_SOURCES = foam_primemod-mcmath.c

# Use the common libs / header files
foamcs_mcmath_SOURCES += $(FRAME_SRC) $(FRAME_HDR)

# Use some modules
foamcs_mcmath_SOURCES += foam_modules-sh.c foam_modules-calib.c foam_modules-img.c foam_modules-log.c
if !SIMHW
foamcs_mcmath_SOURCES += foam_modules-itifg.c foam_modules-okodm.c foam_modules-serial.c foam_modules-daq2k.c
endif !SIMHW

if OPENGL
foamcs_mcmath_SOURCES += foam_modules-dispgl.c
else !OPENGL
foamcs_mcmath_SOURCES += foam_modules-dispsdl.c
endif !OPENGL

foamcs_mcmath_SOURCES += foam_modules-sh.h foam_modules-calib.h foam_modules-img.h foam_modules-log.h
if !SIMHW
foamcs_mcmath_SOURCES += foam_modules-itifg.h foam_modules-okodm.h foam_modules-serial.h foam_modules-daq2k.h foam_modules-dispcommon.h
endif !SIMHW

# Make sure we include the prototypes for functions and datatypes everytime
foamcs_mcmath_CPPFLAGS = -include "foam_primemod-mcmath.h"
#foamcs_mcmath_CPPFLAGS += -DDEBUG_ITIFG=255 
foamcs_mcmath_CPPFLAGS += -DFOAM_MCMATH_DISPLAY=1 -DFOAM_MODULES_DISLAY_SHSUPPORT=1
if SIMHW
foamcs_mcmath_CPPFLAGS += -DFOAM_SIMHW=1
endif SIMHW

foamcs_mcmath_CFLAGS = $(MCMATH_CFLAGS)
foamcs_mcmath_LDFLAGS = $(MCMATH_LIBS)

# Add some cflags and libs too, please (if not simulating the hardware)
if !SIMHW
DRIVERDIR=/home/tim/foam/drivers
foamcs_mcmath_CFLAGS += $(MCMATH_CFLAGS) -I$(DRIVERDIR)/itifg/include -I$(DRIVERDIR)/daq2k/include
foamcs_mcmath_LDFLAGS += $(MCMATH_LIBS) -ldaqx -L$(DRIVERDIR)/itifg/lib -L$(DRIVERDIR)/daq2k/lib
endif !SIMHW
foamcs_mcmath_CPPFLAGS += -DFOAM_MODULES_DISPLAY_OPENGL=1

if OPENGL
foamcs_mcmath_LDFLAGS += $(OPENGL_LIBS)
foamcs_mcmath_CFLAGS += $(OPENGL_CFLAGS)
endif OPENGL

# we need to include this static library last, otherwise we get errors
if !SIMHW
foamcs_mcmath_LDADD = -litifg
endif !SIMHW

endif MCMATH
### END MCMATH MODE ### 

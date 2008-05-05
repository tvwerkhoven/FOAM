# init the bin_programs variable
bin_PROGRAMS = 

# some shared compiler flags which must always be used
# (not using AM_* flags because of http://sourceware.org/automake/automake.html#Flag-Variables-Ordering)
CFLAGS=-pipe -Wall -Wextra
#-O3 -ftree-vectorize
LDFLAGS=$(COMMON_LIBS) -lc

# (un)comment this to toggle GSL inlining
CPPFLAGS+=-DHAVE_INLINE -DGSL_RANGE_CHECK_OFF

# (un)comment this to toggle debugging
#CFLAGS+=-g -DDEBUG_ITIFG=255
# (un)comment this to toggle malloc debugging
HAVE_DMALLOC=1
#LDFLAGS+=-ldmalloc 

# extra files that are not compiled
# EXTRA_DIST =

# basic framework files which are always needed
FRAME_SRC = foam_cs.c foam_library.c foam_cs_library.c
FRAME_HDR = foam_cs_library.h foam_library.h foam_cs_config.h

# dummy moduleset
bin_PROGRAMS += foamcs-dummy
foamcs_dummy_SOURCES = foam_primemod-dummy.c
foamcs_dummy_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_dummy_SOURCES += foam_primemod-dummy.h
foamcs_dummy_CPPFLAGS = -include "foam_primemod-dummy.h"
foamcs_dummy_CFLAGS = -g

# did we enable UI support or not?
if UI
bin_PROGRAMS += foamui
foamui_SOURCES = foam_ui.c foam_ui_library.c  foam_library.c 
foamui_SOURCES += foam_ui_library.h foam_library.h
foamui_CFLAGS = $(UI_CFLAGS)
foamui_LDFLAGS = $(UI_LIBS)
endif

# programs depend on whether or not we want static simulation...
if STATSIMULATION
bin_PROGRAMS += foamcs-statsim
foamcs_statsim_SOURCES = foam_primemod-simstatic.c
foamcs_statsim_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_statsim_SOURCES += foam_modules-sh.c foam_modules-calib.c foam_modules-display.c foam_modules-img.c
foamcs_statsim_SOURCES += foam_modules-sh.h foam_modules-calib.h foam_modules-display.h foam_modules-img.h foam_primemod-simstatic.h
foamcs_statsim_CPPFLAGS = -include "foam_primemod-simstatic.h"
foamcs_statsim_CFLAGS = $(SIMULATION_CFLAGS)
foamcs_statsim_LDFLAGS = $(SIMULATION_LIBS)
endif

# programs depend on whether or not we want simulation...
if SIMULATION

# simulate using only the TT mirror
bin_PROGRAMS += foamcs-simtt
foamcs_simtt_SOURCES = foam_primemod-sim.c
foamcs_simtt_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_simtt_SOURCES += foam_modules-dm.c foam_modules-sh.c foam_modules-sim.c foam_modules-display.c foam_modules-calib.c foam_modules-img.c 
foamcs_simtt_SOURCES += foam_modules-dm.h foam_modules-sh.h foam_modules-sim.h foam_modules-display.h foam_modules-calib.h foam_modules-img.h foam_primemod-sim-tt.h
foamcs_simtt_CPPFLAGS = -include "foam_primemod-sim-tt.h"
foamcs_simtt_CFLAGS = $(SIMULATION_CFLAGS)
foamcs_simtt_LDFLAGS = $(SIMULATION_LIBS)

# simulate using only the DM mirror
bin_PROGRAMS += foamcs-simdm
foamcs_simdm_SOURCES =  foam_primemod-sim.c
foamcs_simdm_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_simdm_SOURCES += foam_modules-dm.c foam_modules-sh.c foam_modules-sim.c foam_modules-display.c foam_modules-calib.c foam_modules-img.c
foamcs_simdm_SOURCES += foam_modules-dm.h foam_modules-sh.h foam_modules-sim.h foam_modules-display.h foam_modules-calib.h foam_modules-img.h foam_primemod-sim-dm.h
foamcs_simdm_CPPFLAGS = -include "foam_primemod-sim-dm.h"
foamcs_simdm_CFLAGS = $(SIMULATION_CFLAGS)
foamcs_simdm_LDFLAGS = $(SIMULATION_LIBS)
endif

# Compile some debug programs for the hardware (okodm, daq, serial, itifg)
#if DEBUG
#bin_PROGRAMS += foamcs-debug-okodm
#foamcs_debug_okodm_SOURCES = foam_modules-okodm.c
#foamcs_debug_okodm_CPPFLAGS = -D

#endif


if MCMATH
# Compile mcmath site specific prime module
bin_PROGRAMS += foamcs-mcmath

# Use the prime module C-file
foamcs_mcmath_SOURCES = foam_primemod-mcmath.c

# Use the common libs / header files
foamcs_mcmath_SOURCES += $(FRAME_SRC) $(FRAME_HDR)

# Use some modules
foamcs_mcmath_SOURCES += foam_modules-sh.c foam_modules-calib.c foam_modules-img.c foam_modules-itifg.c foam_modules-okodm.c foam_modules-serial.c foam_modules-daq2k.c
foamcs_mcmath_SOURCES += foam_modules-sh.h foam_modules-calib.h foam_modules-img.h

# Make sure we include the prototypes for functions and datatypes everytime
foamcs_mcmath_CPPFLAGS = -include "foam_primemod-mcmath.h"

# Add some cflags and libs too, please
foamcs_mcmath_CFLAGS = $(MCMATH_CFLAGS)
foamcs_mcmath_LDFLAGS = $(MCMATH_LIBS)

# endif MCMATH
endif

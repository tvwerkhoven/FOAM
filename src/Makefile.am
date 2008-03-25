# init the bin_programs variable
bin_PROGRAMS = 

# some shared compiler flags which must always be used
# (not using AM_* flags because of http://sourceware.org/automake/automake.html#Flag-Variables-Ordering)
CFLAGS=-pipe -Wall -Wextra -O3 -ftree-vectorize
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
FRAME_HDR = foam_cs_library.h foam_library.h

# dummy moduleset
bin_PROGRAMS += foamcs-dummy
foamcs_dummy_SOURCES = foam_primemod-dummy.c
foamcs_dummy_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_dummy_SOURCES += foam_primemod-dummy.h
foamcs_dummy_CPPFLAGS = -include "foam_primemod-dummy.h"

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
foamcs_statsim_SOURCES += foam_modules-sh.c foam_modules-calib.c foam_modules-display.c foam_modules-pgm.c
foamcs_statsim_SOURCES += foam_modules-sh.h foam_modules-calib.h foam_modules-display.h foam_modules-pgm.h foam_primemod-simstatic.h
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
foamcs_simtt_SOURCES += foam_modules-dm.c foam_modules-sh.c foam_modules-sim.c foam_modules-display.c foam_modules-calib.c foam_modules-pgm.c 
foamcs_simtt_SOURCES += foam_modules-dm.h foam_modules-sh.h foam_modules-sim.h foam_modules-display.h foam_modules-calib.h foam_modules-pgm.h foam_primemod-sim-tt.h
foamcs_simtt_CPPFLAGS = -include "foam_primemod-sim-tt.h"
foamcs_simtt_CFLAGS = $(SIMULATION_CFLAGS)
foamcs_simtt_LDFLAGS = $(SIMULATION_LIBS)

# simulate using only the DM mirror
bin_PROGRAMS += foamcs-simdm
foamcs_simdm_SOURCES =  foam_primemod-sim.c
foamcs_simdm_SOURCES += $(FRAME_SRC) $(FRAME_HDR)
foamcs_simdm_SOURCES += foam_modules-dm.c foam_modules-sh.c foam_modules-sim.c foam_modules-display.c foam_modules-calib.c foam_modules-pgm.c
foamcs_simdm_SOURCES += foam_modules-dm.h foam_modules-sh.h foam_modules-sim.h foam_modules-display.h foam_modules-calib.h foam_modules-pgm.h foam_primemod-sim-dm.h
foamcs_simdm_CPPFLAGS = -include "foam_primemod-sim-dm.h"
foamcs_simdm_CFLAGS = $(SIMULATION_CFLAGS)
foamcs_simdm_LDFLAGS = $(SIMULATION_LIBS)
endif

if ITIFG
# this does not work ATM, no useful modules present
bin_PROGRAMS += foamcs-itifg
foamcs_itifg_SOURCES = foam_cs.c foam_library.c foam_cs_library.c
foamcs_itifg_SOURCES += foam_library.h foam_cs_library.h
foamcs_itifg_CPPFLAGS = -include "foam_modules-itifg.h"
foamcs_itifg_CFLAGS = $(ITIFG_CFLAGS)
foamcs_itifg_LDFLAGS = $(ITIFG_LIBS)
endif

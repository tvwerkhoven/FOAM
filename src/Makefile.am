# init the bin_programs variable
bin_PROGRAMS = 

# some shared compiler flags which must always be used
# (not using AM_* flags because of http://sourceware.org/automake/automake.html#Flag-Variables-Ordering)
CFLAGS=-pipe -Wall -Wextra -std=c99 -O3 -ftree-vectorize
LDFLAGS=$(COMMON_LIBS) -lc

# (un)comment this to toggle debugging
CFLAGS+=-g -DDEBUG_ITIFG=255
# (un)comment this to toggle malloc debugging
HAVE_DMALLOC=1
LDFLAGS+=-ldmalloc

# extra files buiten compilen om
# EXTRA_DIST =

# dummy moduleset
bin_PROGRAMS += foamcs-dummy
foamcs_dummy_SOURCES = foam_cs.c foam_library.c foam_cs_library.c foam_modules-dummy.c
foamcs_dummy_SOURCES += foam_cs_library.h foam_library.h


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
foamcs_statsim_SOURCES = foam_cs.c foam_library.c foam_cs_library.c foam_modules-sh.c foam_modules-simstatic.c foam_modules-display.c foam_modules-calib.c
foamcs_statsim_SOURCES += foam_cs_library.h foam_library.h foam_modules-sh.h foam_modules-sim.h foam_modules-display.h
foamcs_statsim_CFLAGS = $(SIMULATION_CFLAGS)
foamcs_statsim_LDFLAGS = $(SIMULATION_LIBS)
endif


# programs depend on whether or not we want simulation...
if SIMULATION
bin_PROGRAMS += foamcs-sim
foamcs_sim_SOURCES = foam_cs.c foam_library.c foam_cs_library.c foam_modules-dm.c  foam_modules-sh.c foam_modules-sim.c foam_modules-display.c foam_modules-calib.c
foamcs_sim_SOURCES += foam_cs_library.h foam_library.h foam_modules-sh.h foam_modules-sim.h foam_modules-display.h
foamcs_sim_CPPFLAGS = -include "foam_modules-sim.h"
foamcs_sim_CFLAGS = $(SIMULATION_CFLAGS)
foamcs_sim_LDFLAGS = $(SIMULATION_LIBS)
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

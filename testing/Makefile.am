# init the bin_programs variable
bin_PROGRAMS = serial-test okodm-test daq2k-test itifg-test

# some shared compiler flags which must always be used
# (not using AM_* flags because of http://sourceware.org/automake/automake.html#Flag-Variables-Ordering)
CFLAGS=-pipe -Wall -Wextra
LDFLAGS=$(COMMON_LIBS) -lc

# (un)comment this to toggle GSL inlining
CPPFLAGS+=-DHAVE_INLINE -DGSL_RANGE_CHECK_OFF

# (un)comment this to toggle debugging
CFLAGS+=-g -DDEBUG_ITIFG=255
# (un)comment this to toggle malloc debugging
HAVE_DMALLOC=1
#LDFLAGS+=-ldmalloc 

SRCDIR=$(abs_top_srcdir)/src



# serial module test
serial_test_SOURCES = $(SRCDIR)/foam_modules-serial.c
serial_test_CPPFLAGS = -DFOAM_MODSERIAL_ALONE=1
serial_test_CFLGAGS = -Wall -std=c99

# okodm test
okodm_test_SOURCES = $(SRCDIR)/foam_modules-okodm.c
okodm_test_CPPFLAGS = -DFOAM_MODOKODM_ALONE=1
okodm_test_CFLAGS = -Wall -std=c99
okodm_test_LDFLAGS = -lm -lc -lgslcblas -lgsl

# daq test
daq2k_test_SOURCES = $(SRCDIR)/foam_modules-daq2k.c
daq2k_test_CPPFLAGS = -DFOAM_MODDAQ2K_ALONE=1 -I/home/tim/foam/drivers/daq2k/include
daq2k_test_LDFLAGS = -lc -lm -ldaqx -L/home/tim/foam/drivers/lib
daq2k_test_CFLAGS = -std=c99

# itifg test
itifg_test_SOURCES = $(SRCDIR)/foam_modules-img.c $(SRCDIR)/foam_modules-itifg.c
itifg_test_CPPFLAGS = -DFOAM_MODITIFG_ALONE=1 `sdl-config --cflags` -I/home/tim/foam/drivers/itifg/include
itifg_test_LDFLAGS = -lc -lm `sdl-config --libs` -L/home/tim/foam/drivers/itifg/lib
itifg_test_CFLAGS = -Wall



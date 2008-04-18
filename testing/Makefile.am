# init the bin_programs variable
bin_PROGRAMS = serial-test okodm-test daq2k-test modimg-test
# itifg-test done manually

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

# we don't need or use the common libs here
LIBS=

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
daq2k_test_CPPFLAGS = -DFOAM_MODDAQ2K_ALONE=1 -Idaq2k/include
daq2k_test_LDFLAGS = -lc -lm -ldaqx -Ldaq2k/lib
daq2k_test_CFLAGS = -std=c99

# img module test
modimg_test_SOURCES = $(SRCDIR)/foam_modules-img.c
modimg_test_CPPFLAGS = -DFOAM_MODIMG_ALONE=1
modimg_test_LDFLAGS = -lc -lm  `sdl-config --libs`
modimg_test_CFLAGS = -std=c99 `sdl-config --cflags`

# itifg test
#itifg_test_SOURCES = $(SRCDIR)/foam_cs_library.c $(SRCDIR)/foam_modules-img.c $(SRCDIR)/foam_modules-itifg.c
#itifg_test_CPPFLAGS = -DFOAM_MODITIFG_ALONE=1 -DLinux `sdl-config --cflags` -I/home/tim/foam/drivers/itifg/include
#itifg_test_LDFLAGS = -lm -lc -lgd -lpng -lSDL_image `sdl-config --libs` /home/tim/foam/drivers/itifg/lib/libitifg.a 
#itifg_test_CFLAGS = -Wall -g 

# here comes the itifg hack:
itifg-test: $(SRCDIR)/foam_modules-itifg.c
	$(CC) $(CFLAGS) $(GFLAGS) $(LIFLAGS) $(LLFLAGS) \
	-Iitifg/include  \
	-Litifg/lib  \
	-I../src/ \
	-include "foam_modules-alone.h" \
	-DFOAM_MODITIFG_ALONE=1 \
	-o $@ $^ \
	-litifg -lm -lc -g \
	-lgd -lpng -lSDL_image `sdl-config --libs --cflags`

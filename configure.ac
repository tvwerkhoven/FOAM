## @file configure.ac
## @author Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
## Copyright 2007-2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>

AC_PREREQ(2.59)
m4_include([version.m4])

AC_INIT([FOAM], [GIT_REVISION], [Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>])
AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_SRCDIR(foam/foam.cc)
AC_CONFIG_HEADERS(foam/autoconfig.h)

#AM_INIT_AUTOMAKE([AC_PACKAGE_NAME], [AC_PACKAGE_VERSION])
AM_INIT_AUTOMAKE

AC_COPYRIGHT([Copyright (C) 2007-2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>])
AC_REVISION([GIT_REVISION])


AC_CONFIG_FILES([
Makefile
foam/Makefile
ui/Makefile
conf/Makefile
data/Makefile
testing/Makefile
testing_devs/Makefile
docs/Makefile
])

AC_GNU_SOURCE

AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB

PKG_PROG_PKG_CONFIG
AC_CANONICAL_HOST

# Check build host (might be superfluous?)
case "$build" in
	*-*-linux*)
	have_linux=true
	;;
	*-*-darwin*)
	have_macosx=true
	;;
esac
AM_CONDITIONAL(HAVE_LINUX, test x$have_linux = xtrue)
AM_CONDITIONAL(HAVE_MACOSX, test x$have_macosx = xtrue)

# FOAM data & configuration directory

### CHECK FOR REQUIRED LIBRARIES ###
####################################

### Store & reset $(LIBS) to capture output of AC_SEARCH_LIBS
OLDLIBS=$LIBS
LIBS=

### These libraries are crucial for all targets
AC_SEARCH_LIBS([sin],
		[m],
		[],
		[FOAM_FATAL=yes;FOAM_MISSING+=" libm"])

AC_SEARCH_LIBS([pthread_create],
		[pthread],
		[],
		[FOAM_FATAL=yes;FOAM_MISSING+=" pthread"])

AC_SEARCH_LIBS([ffopen],
		[cfitsio],
  	[AC_DEFINE([HAVE_CFITSIO], [1], [CFITSIO supported.])],
		[FOAM_FATAL=yes;FOAM_MISSING+=" cfitsio"])

PKG_CHECK_MODULES(SIGC, 
		[sigc++-2.0 >= 2.0], 
		[],
		[FOAM_FATAL=yes;FOAM_MISSING+=" sigc++-2.0"])

PKG_CHECK_MODULES(GSL, 
		[gsl >= 1.11], 
		[AC_DEFINE([HAVE_GSL], [1], [GSL supported.])],
		[FOAM_FATAL=yes;FOAM_MISSING+=" gsl gslcblas";])

AS_IF([test "$FOAM_FATAL" = yes],
	[AC_MSG_ERROR([Cannot build FOAM! Missing libraries:$FOAM_MISSING!])],
	[])

#PKG_CHECK_MODULES(FITSIO, 
#		[cfitsio], 
#		[AC_DEFINE([HAVE_CFITSIO], [1], [CFITSIO supported.])],
#		[FOAM_FATAL=yes;FOAM_MISSING+=" cfitsio"])

AC_SUBST(COMMON_CFLAGS, ["$SIGC_CFLAGS $GSL_CFLAGS $FITSIO_CFLAGS"])
AC_SUBST(COMMON_LIBS, ["$SIGC_LIBS $GSL_LIBS $FITSIO_LIBS $LIBS"])

# Reset LIBS
LIBS=$OLDLIBS

### CHECK FOR OPTIONAL LIBRARIES ###
####################################

OLDLIBS=$LIBS
LIBS=

# Optional libraries, for libimgdata.a.
# @todo this should be handled by libsiu (through .pc file?)
AC_SEARCH_LIBS([deflate], [z],
		[have_z=yes],
		[have_z=no])

AC_SEARCH_LIBS([IcsOpen], [ics],
		[have_ics=yes],
		[have_ics=no])

LIBIMGDATA_LIBS=$LIBS

# Reset LIBS
LIBS=$OLDLIBS

# for fullsim, expoao etc.
PKG_CHECK_MODULES([FFTW], [fftw3 >= 3.1.2], 
		[have_fftw=yes],
		[have_fftw=no])

### INSPECT CONFIGURATION OPTIONS ###
#####################################

AC_ARG_ENABLE([dummy],
		AC_HELP_STRING([--disable-dummy], [don't build dummy mode]),
		[have_dummy=$enableval],
		[have_dummy=yes])
AM_CONDITIONAL([HAVE_DUMMY], [test "x$have_dummy" != "xno"])

AC_ARG_ENABLE([simstat],
		AC_HELP_STRING([--disable-simstat], [don't build static simulation mode]),
		[have_simstat=$enableval],
		[have_simstat=yes])
AM_CONDITIONAL([HAVE_SIMSTAT], [test "x$have_simstat" != "xno"])

AC_ARG_ENABLE([fullsim],
		AC_HELP_STRING([--disable-fullsim], [don't build full simulation mode]),
		[have_fullsim=$enableval],
		[have_fullsim=yes])
AM_CONDITIONAL([HAVE_FULLSIM], [test "x$have_fullsim" != "xno"])

AC_ARG_ENABLE([expoao],
		AC_HELP_STRING([--disable-expoao], [don't build expoao mode]),
		[have_expoao=$enableval],
		[have_expoao=yes])
AM_CONDITIONAL([HAVE_EXPOAO], [test "x$have_expoao" != "xno"])

AC_ARG_ENABLE([gui],
		AC_HELP_STRING([--disable-gui], [don't build FOAM gui]),
		[have_gui=$enableval],
		[have_gui=yes])
AM_CONDITIONAL([HAVE_GUI], [test "x$have_gui" != "xno"])

AC_ARG_ENABLE([hwdebug],
		AC_HELP_STRING([--disable-hwdebug], [don't build hardware debugging tests]),
		[have_hwdebug=$enableval],
		[have_hwdebug=yes])
AM_CONDITIONAL([HAVE_HWDEBUG], [test "x$have_hwdebug" != "xno"])

### Documentation options

AC_ARG_ENABLE([docs],
		AC_HELP_STRING([--enable-docs], [compile documentation]),
		[have_docs=$enableval],
		[have_docs=no])

AC_ARG_ENABLE([extdocs],
		AC_HELP_STRING([--enable-extdocs], [compile verbose documentation]),
		[have_extdocs=$enableval],
		[have_extdocs=no])

### Debug / profiling options ###

AC_ARG_ENABLE([debug],
		AC_HELP_STRING([--enable-debug], [enable debug output]),
		[have_debug=$enableval],
		[have_debug=no])
AC_ARG_ENABLE([strict-debug],
		AC_HELP_STRING([--enable-strict-debug], [enable strict (agressive) debug output]),
		[have_str_debug=$enableval;],
		[have_str_debug=no])
AM_CONDITIONAL([HAVE_STR_DEBUG], [test "x$have_str_debug" != "xno"])
AS_IF([test "x$have_str_debug" != "xno"],
		[
		AC_MSG_WARN([*** Enabling strict debugging. Continue at your own risk! (but in any case make sure to use strict debugging consistently)])
		have_debug=yes
		])
AM_CONDITIONAL([HAVE_DEBUG], [test "x$have_debug" != "xno"])
AS_IF([test "x$have_debug" != "xno"],
		[AC_DEFINE([FOAM_DEBUG], [1], [Toggle debugging.])],
		[AC_DEFINE([FOAM_DEBUG], [0], [Toggle debugging.])])

AC_ARG_ENABLE([profiling],
		AC_HELP_STRING([--enable-profiling], [enable profiling support]),
		[have_profiling=$enableval],
		[have_profiling=no])
AM_CONDITIONAL([HAVE_PROFILING], [test "x$have_profiling" != "xno"])

### Compilation dependencies

AC_ARG_WITH([libsiu],
		[AC_HELP_STRING([--with-libsiu], [location of libsiu (def: ./libsiu)])],
		[have_libsiu=$withval],
		[have_libsiu="./libsiu"])

### CHECK LIBSIU LIBRARY ###
############################

### Libsiu testing, 1) test if pthread++.h exist, 2) test for libsighandle.a
AC_MSG_NOTICE([*** Looking for libsiu library.])

AC_CHECK_FILE([$have_libsiu/pthread++.h],
		[],
		[AC_MSG_ERROR([*** libsiu not found in $have_libsiu. Please install first (see INSTALL)])])
AC_CHECK_FILE([$have_libsiu/libsighandle.a],
		[],
		[AC_MSG_ERROR([*** libsiu files found, but could find compiled libraries. Please install first (see INSTALL)])])

AC_SUBST(LIBSIU, [$have_libsiu])

### CHECK FOR HARDWARE LIBRARIES ###
####################################

AC_MSG_NOTICE([*** Checking for 1394 camera support.])

PKG_CHECK_MODULES(DC1394, [libdc1394-2 >= 2.0.2],
		[have_dc1394=yes],
		[have_dc1394=no;dc1394_missing+=" libdc1394"])

# TODO: need to check things for linux and mac here
# TODO: test this, do we need it?
AS_IF([test "x$have_dc1394" != "xno"],
	AS_CASE([$build], 
			[*-*-linux*], 
			[
			AC_SEARCH_LIBS([raw1394_channel_modify], [raw1394],
				[have_dc1394=yes],
				[have_dc1394=no;hw1394_missing+=" raw1394"])
			], 
			[*-*-darwin*],
			[
			AC_SEARCH_LIBS([IOMasterPort], [IOKit],
				[have_dc1394=yes],
				[have_dc1394=no;hw1394_missing+=" IOKit"])
			], [])
	[])

AM_CONDITIONAL([HAVE_DC1394], [test "x$have_dc1394" != "xno"])

AC_MSG_NOTICE([*** Checking for Andor camera support.])

AC_CHECK_HEADERS([atmcdLXd.h], 
		[have_andor_h=yes], 
		[have_andor_h=no;hwandor_missing+=" atmcdLXd.h"])

OLDLIBS=$LIBS
LIBS=

AC_SEARCH_LIBS([Initialize],
		[andor],
		[have_andor_lib=yes],
		[have_andor_lib=no;hwandor_missing+=" libandor"])

AC_SUBST(ANDORCAM_LIBS, ["$LIBS"])

# Reset LIBS back to old value
LIBS=$OLDLIBS

AS_IF([test "x$have_andor_h" != "xyes" || test  "x$have_andor_lib" != "xyes"],
		[have_andor=no],
		[have_andor=yes])
AM_CONDITIONAL([HAVE_ANDORCAM], [test "x$have_andor" != "xno"])

AC_MSG_NOTICE([*** Checking for Alpao DM support.])

AC_CHECK_HEADERS([acecs.h], [], [have_alpaodm_h=no;hwalpaodm_missing+=" acecs.h"])
AC_CHECK_HEADERS([acedev5.h], [], [have_alpaodm_h=no;hwalpaodm_missing+=" acedev5.h"])

OLDLIBS=$LIBS
LIBS=

AC_SEARCH_LIBS([Register_Card],
		[pci_dask],
		[],
		[have_alpaodm_lib=no;hwalpaodm_missing+=" pci_dask"])

AC_SEARCH_LIBS([acecsErrDisplay],
		[acecs],
		[],
		[have_alpaodm_lib=no;hwalpaodm_missing+=" acecs"])

AC_SEARCH_LIBS([acedev5Send],
		[acedev5],
		[],
		[have_alpaodm_lib=no;hwalpaodm_missing+=" acedev5"])

AC_SUBST(ALPAODM_LIBS, ["$LIBS"])

# Reset LIBS back to old value
LIBS=$OLDLIBS

AS_IF([test "x$have_alpaodm_h" != "xno" || test  "x$have_alpaodm_lib" != "xno"],
		[have_alpaodm=yes],
		[have_alpaodm=no])
AM_CONDITIONAL([HAVE_ALPAODM], [test "x$have_alpaodm" != "xno"])

### DOCUMENTATION BUILD CHECKS ###
##################################

AS_IF([test "x$have_docs" != "xno"],
		[
		AC_MSG_NOTICE([*** Checking documentation tools.])
		AC_CHECK_PROG(have_doxygen, doxygen, yes, no)
		AS_IF([test "x$have_doxygen" != "xyes"], [docsmissing+=" doxygen";have_docs=no], [])
		AC_CHECK_PROG(have_dot, dot, yes, no)
		AS_IF([test "x$have_dot" != "xyes"], [docsmissing+=" dot";have_docs=no], [])
		],
		[AC_MSG_NOTICE([*** Not compiling documentation])])

AM_CONDITIONAL([HAVE_DOCS], [test "x$have_docs" != "xno"])
AM_CONDITIONAL([HAVE_EXTDOCS], [test "x$have_extdocs" != "xno" && test "x$have_docs" != "xno"])

### STATIC SIMULATION MODE LIBS ###
###################################

AM_CONDITIONAL([HAVE_SIMSTAT], [test "x$have_simstat" != "xno"])

### FULL SIMULATION MODE LIBS ###
#################################

AS_IF([test "x$have_fullsim" != "xno"],
		[
		AC_MSG_NOTICE([*** Full simulation enabled, checking for extra libs])

		AS_IF([test "x$have_fftw" != "xyes"], [have_fullsim=no;fullsimmissing+=" fftw3"])

		AC_SUBST(FULLSIM_CFLAGS, ["$FFTW_CFLAGS"])
		AC_SUBST(FULLSIM_LIBS, ["$FFTW_LIBS $LIBIMGDATA_LIBS"])
	],
	[AC_MSG_NOTICE([*** Full simulation disabled])])

AM_CONDITIONAL([HAVE_FULLSIM], [test "$have_fullsim" != "xno"])

### EXPOAO BUILD TARGET LIBS ###
################################

AS_IF([test "x$have_expoao" != "xno"],
		[
		AC_MSG_NOTICE([*** Expoao target enabled, checking for libs])

		# Check if we have the required libraries (fftw3, andorcam)
		AS_IF([test "x$have_fftw" != "xyes"], [have_expoao=no;expoaomissing+=" fftw3"])
		AS_IF([test "x$have_andor" != "xyes"], [have_expoao=no;expoaomissing+=" andor"])
		AS_IF([test "x$have_alpaodm" != "xyes"], [have_expoao=no;expoaomissing+=" alpaodm"])

		AC_SUBST(EXPOAO_CFLAGS, ["$FFTW_CFLAGS $LIBIMGDATA_CFLAGS"])
		AC_SUBST(EXPOAO_LIBS, ["$FFTW_LIBS $LIBIMGDATA_LIBS $ANDORCAM_LIBS $ALPAODM_LIBS"])
	],
	[AC_MSG_NOTICE([*** Expoao target disabled])])

AM_CONDITIONAL([HAVE_EXPOAO], [test "x$have_expoao" != "xno"])

### GUI LIBRARIES ###
#####################

# Check for opengl support
have_opengl=yes
AS_IF([test "x$have_gui" != "xno"],
		[
		AX_CHECK_GL
		AS_IF([test "x$no_gl" = "xyes"], [have_opengl=no;glmissing+=" gl"])
#		AX_CHECK_GLU
#		AS_IF([test "x$no_glu" = "xyes"], [have_opengl=no;glmissing+=" glu"])
		AX_CHECK_GLUT
		AS_IF([test "x$no_glut" = "xyes"], [have_opengl=no;glmissing+=" glut"])
		], [])

AM_CONDITIONAL([HAVE_OPENGL], [test "x$have_opengl" != "xno"])

# Check for GUI support
AS_IF([test "x$have_gui" != "xno"],
		[
		AC_MSG_NOTICE([*** GUI enabled, checking for extra libs])
		PKG_CHECK_MODULES(GTKMM, gtkmm-2.4 >= 2.6, [],
				[have_gui=no;guimissing+=" gtkmm-2.4"])
		PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16.6, [],
				[have_gui=no;guimissing+=" gthread-2.0"])
		PKG_CHECK_MODULES(GLEXTMM, gtkglextmm-1.2 >= 1.2.0, [],
				[have_gui=no;guimissing+=" gtkglextmm-1.2"])
		AS_IF([test "x$have_opengl" = "xno"], 
				[have_gui=no;guimissing+=" $glmissing"],
				[])

		AC_SUBST(GUI_CFLAGS, ["$GTHREAD_CFLAGS $GTKMM_CFLAGS $GL_CFLAGS $GLUT_CFLAGS $GLEXTMM_CFLAGS"])
		AC_SUBST(GUI_LIBS, ["$GTHREAD_LIBS $GTKMM_LIBS $GL_LIBS $GLUT_LIBS $GLEXTMM_LIBS"])
		],
		[AC_MSG_NOTICE([*** GUI disabled])])

AM_CONDITIONAL([HAVE_GUI], [test "x$have_gui" != "xno"])

### END CHECKING FOR LIBS ###
#############################

AC_MSG_NOTICE([===============Summary================])


AC_MSG_NOTICE([---------------Hardware---------------])

AC_MSG_NOTICE([1394 cameras:            $have_dc1394 $hw1394_missing])
AC_MSG_NOTICE([Andor cameras:           $have_andor $hwandor_missing])
AC_MSG_NOTICE([Alpao DM:                $have_alpaodm $hwalpaodm_missing])

AC_MSG_NOTICE([---------------Targets----------------])

AC_MSG_NOTICE([Dummy mode:              $have_dummy])
AC_MSG_NOTICE([Static sim mode:         $have_simstat])
AC_MSG_NOTICE([Full sim mode:           $have_fullsim $fullsimmissing])

AC_MSG_NOTICE([ExPoAO build:            $have_expoao $expoaomissing])

AC_MSG_NOTICE([FOAM control GUI:        $have_gui $guimissing])

AC_MSG_NOTICE([---------------Debugging--------------])

AC_MSG_NOTICE([Debug enabled:           $have_debug])
AC_MSG_NOTICE([Strict debug enabled:    $have_str_debug])

AC_MSG_NOTICE([Profiling enabled:       $have_profiling])

AC_MSG_NOTICE([Hardware debug targets:  $have_hwdebug])

AC_MSG_NOTICE([---------------Documentation----------])

AC_MSG_NOTICE([FOAM docs:               $have_docs $docsmissing])
AC_MSG_NOTICE([FOAM extdocs:            $have_extdocs])

AC_MSG_NOTICE([---------------Dependencies-----------])

AC_MSG_NOTICE([libsiu:                  $have_libsiu])

AC_MSG_NOTICE([======================================])


AC_OUTPUT

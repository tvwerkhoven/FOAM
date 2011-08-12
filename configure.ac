## @file configure.ac
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
## Copyright 2007-2011 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

AC_PREREQ(2.59)
m4_include([version.m4])

AC_INIT([FOAM], [GIT_REVISION], [Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([AC_PACKAGE_NAME], [AC_PACKAGE_VERSION])

AC_COPYRIGHT([Copyright (C) 2007-2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>])
AC_REVISION([GIT_REVISION])

AC_CONFIG_SRCDIR(foam/foam.cc)
AC_CONFIG_HEADERS(foam/autoconfig.h)

AC_CONFIG_FILES([
Makefile
foam/Makefile
ui/Makefile
conf/Makefile
data/Makefile
testing/Makefile
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
#FOAMDATADIR=$datadir/foam
#FOAMCONFDIR=$sysconfdir/foam
#AC_SUBST(FOAMDATADIR)
#AC_SUBST(FOAMCONFDIR)

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

PKG_CHECK_MODULES(SIGC, [sigc++-2.0 >= 2.0], [],
		[FOAM_FATAL=yes;FOAM_MISSING+=" sigc++-2.0"])

PKG_CHECK_MODULES(GSL, [gsl >= 1.11], [AC_DEFINE([HAVE_GSL], [1], [GSL supported.])],
		[FOAM_FATAL=yes;FOAM_MISSING+=" gsl gslcblas";])

#PKG_CHECK_MODULES(FITSIO, [cfitsio], [AC_DEFINE([HAVE_CFITSIO], [1], [CFITSIO supported.])],
#		[FOAM_FATAL=yes;FOAM_MISSING+=" cfitsio"])

COMMON_CFLAGS="$SIGC_CFLAGS $GSL_CFLAGS $FITSIO_CFLAGS"
COMMON_LIBS="$SIGC_LIBS $GSL_LIBS $FITSIO_LIBS $LIBS"
AC_SUBST(COMMON_CFLAGS)
AC_SUBST(COMMON_LIBS)

# Reset LIBS
LIBS=$OLDLIBS

### INSPECT BUILD TARGETS ###
#############################

dummy=true
AC_ARG_ENABLE([dummy],
		AC_HELP_STRING([--disable-dummy], [don't build dummy mode]),
		[test "$enableval" = no && dummy=disabled], [])

docs=true
AC_ARG_ENABLE([docs],
		AC_HELP_STRING([--disable-docs], [don't compile documentation]),
		[test "$enableval" = no && docs=disabled], [])

extdocs=false
AC_ARG_ENABLE([extdocs],
		AC_HELP_STRING([--enable-extdocs], [compile extended documentation]),
		[test "$enableval" = yes && extdocs=true], [extdocs=disabled])

simstat=true
AC_ARG_ENABLE([simstat],
		AC_HELP_STRING([--disable-simstat], [don't build static simulation mode]),
		[test "$enableval" = no && simstat=disabled], [])

fullsim=true
AC_ARG_ENABLE([fullsim],
		AC_HELP_STRING([--disable-fullsim], [don't build full simulation mode]),
		[test "$enableval" = no && fullsim=disabled], [])

debug=true
AC_ARG_ENABLE([debug],
		AC_HELP_STRING([--disable-debug], [don't disable debugging mode]),
		[test "$enableval" = no && debug=disabled], [])

gui=true
AC_ARG_ENABLE([gui],
		AC_HELP_STRING([--disable-gui], [don't build gui]),
		[test "$enableval" = no && gui=disabled], [])

AC_ARG_ENABLE([hwdebug],
		AC_HELP_STRING([--enable-hwdebug], [build hardware debugging tests]),
		[test "$enableval" = yes && hwdebug=true], [hwdebug=disabled])

### INSPECT BUILD OPTIONS ###
#############################
		
AC_ARG_WITH([libsiu],
		AC_HELP_STRING([--with-libsiu], [location of libsiu (def: ./libsiu)]),
		[test "$withval" != false && LIBSIU=$withval],
		[LIBSIU="libsiu"])
		
# @todo test LIBSIU presence
if test -d $LIBSIU ; then
	AC_MSG_NOTICE([*** Found libsiu in $LIBSIU.])
else
	AC_MSG_ERROR([*** libsiu not found in $LIBSIU. Please install first (see INSTALL)])
fi

### Set some automake variables ###
###################################

AM_CONDITIONAL([DEBUG], [test "$debug" = true])
AM_CONDITIONAL([HWDEBUG], [test "$hwdebug" = true])
AC_SUBST(LIBSIU)

### CHECK FOR HARDWARE LIBRARIES ###
####################################
AC_MSG_NOTICE([*** Checking hardware capabilities.])

# Check 1394 camera libraries dc1394
PKG_CHECK_MODULES(DC1394, [libdc1394-2 >= 2.0.2],
		[have_dc1394=true],
		[have_dc1394=false;hw1394_missing+=" libdc1394"])

# TODO: need to check things for linux and mac here
if test "$have_dc1394" = true ; then
	case "$build" in
		*-*-linux*)
		# TODO: test this, do we need it?
		AC_SEARCH_LIBS([raw1394_channel_modify], [raw1394],
				[have_dc1394=true;],
				[have_dc1394=false;hw1394_missing+=" raw1394"])
		;;
		*-*-darwin*)
		AC_SEARCH_LIBS(IOMasterPort, IOKit,
				[have_dc1394=true],
				[have_dc1394=false;hw1394_missing+=" IOKit"])
		;;
	esac
fi
AM_CONDITIONAL(HAVE_DC1394, test x$have_dc1394 = xtrue)

# Check Andor camera libraries (should have atmcdLXd.h and libandor.so)

AC_CHECK_HEADERS([atmcdLXd.h], 
		[have_andor_h=true], 
		[have_andor_h=false;hwandor_missing+=" atmcdLXd.h"])

OLDLIBS=$LIBS
LIBS=

AC_SEARCH_LIBS([Initialize],
		[andor],
		[have_andor_lib=true],
		[have_andor_lib=false;hwandor_missing+=" libandor"])

ANDORCAM_LIBS="$LIBS"
AC_SUBST(ANDORCAM_LIBS)

# Reset LIBS back to old value
LIBS=$OLDLIBS

have_andor=true
if test "$have_andor_h" = false || test  "$have_andor_lib" = false ; then
		have_andor=false
fi

AM_CONDITIONAL(HAVE_ANDORCAM, test x$have_andor = xtrue)

### DOCUMENTATION BUILD CHECKS ###
##################################
AC_MSG_NOTICE([*** Checking documentation tools.])

if test "$docs" = true ; then
	AC_CHECK_PROG(have_doxygen, doxygen, yes, no)
	if test "$have_doxygen" = no; then docsmissing+=" doxygen"; fi
	
	AC_CHECK_PROG(have_dot, dot, yes, no)
	if test "$have_dot" = no; then docsmissing+=" dot"; fi
	
	if test "$have_dot" = no || test  "$have_doxygen" = no ; then
		docs=false;
		AC_MSG_WARN([FOAM docs need doxygen & dot programs to compile]);
	else
		AC_MSG_NOTICE([*** Will compile documentation])
	fi
else
	AC_MSG_NOTICE([*** Not compiling documentation])
fi

AM_CONDITIONAL([HAVE_DOCS], [test "$docs" = true])
AM_CONDITIONAL([HAVE_EXTDOCS], [test "$extdocs" = true])


### STATIC SIMULATION MODE LIBS ###
###################################
if test "$simstat" = true ; then
	AC_MSG_NOTICE([*** Static simulation enabled, doesn't need extra libs])
		
else 
	AC_MSG_NOTICE([*** Static simulation disabled])
fi

AC_SUBST(SIMSTAT_CFLAGS)
AC_SUBST(SIMSTAT_LIBS)
AM_CONDITIONAL([SIMSTAT], [test "$simstat" = true])

### FULL SIMULATION MODE LIBS ###
#################################
if test "$fullsim" = true ; then
	AC_MSG_NOTICE([*** Full simulation enabled, checking for extra libs])
	fullsimmissing=""
	
	# Optional libraries, for libimgdata.a.
	# @todo this should be handled by libsiu (through .pc file?)
	AC_SEARCH_LIBS([deflate], [z],
		[have_z=true],
		[have_z=false])

	AC_SEARCH_LIBS([IcsOpen], [ics],
			[have_ics=true],
			[have_ics=false])

	PKG_CHECK_MODULES(FFTW, fftw3 >= 3.1.2, [],
		[AC_MSG_WARN([FOAM full sim needs fftw3.]);fullsim=false;fullsimmissing+=" fftw3"])
	
	FULLSIM_CFLAGS+=" $FFTW_CFLAGS"
	FULLSIM_LIBS+=" $FFTW_LIBS"
else 
	AC_MSG_NOTICE([*** Full simulation disabled])
fi

AC_SUBST(FULLSIM_CFLAGS)
AC_SUBST(FULLSIM_LIBS)
AM_CONDITIONAL([FULLSIM], [test "$fullsim" = true])


### GUI LIBRARIES ###
#####################
opengl=disabled
if test "$gui" = true ; then
	AC_MSG_NOTICE([*** GUI enabled, checking for extra libs])
	guimissing=""
	opengl=true;
	
	PKG_CHECK_MODULES(GTKMM, gtkmm-2.4 >= 2.6, [],
		[AC_MSG_WARN([FOAM GUI needs gtkmm.]);gui=false;guimissing+=" gtkmm-2.4"])
	
	PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16.6, [],
		[AC_MSG_WARN([FOAM GUI needs gthread-2.0.]);gui=false;guimissing+=" gthread-2.0"])
	
	### OpenGL for UI etc.

	AX_CHECK_GL
	if test "$no_gl" = yes; then guimissing+=" gl"; fi

	AX_CHECK_GLU
	if test "$no_glu" = yes; then guimissing+=" glu"; fi

	AX_CHECK_GLUT
	if test "$no_glut" = yes; then guimissing+=" glut"; fi

	if test "$no_gl" = yes || test  "$no_glu" = yes || test "$no_glut" = yes ; then
		opengl=false;
		gui=false;
		AC_MSG_WARN([FOAM GUI needs OpenGL]);
	fi

	PKG_CHECK_MODULES(GLEXTMM, gtkglextmm-1.2 >= 1.2.0, [],
		[AC_MSG_WARN([FOAM GUI gtkglextmm-1.2.]);gui=false;guimissing+=" gtkglextmm-1.2"])

	GUI_CFLAGS+=" $GTHREAD_CFLAGS $GTKMM_CFLAGS $GL_CFLAGS $GLU_CFLAGS $GLUT_CFLAGS $GLEXTMM_CFLAGS"
	GUI_LIBS+=" $GTHREAD_LIBS $GTKMM_LIBS $GL_LIBS $GLU_LIBS $GLUT_LIBS $GLEXTMM_LIBS"
else 
	AC_MSG_NOTICE([*** GUI disabled])
fi

AC_SUBST(GUI_CFLAGS)
AC_SUBST(GUI_LIBS)
AM_CONDITIONAL([GUI], [test "$opengl" = true])
AM_CONDITIONAL([HAVE_OPENGL], [test "$opengl" = true])

### END CHECKING FOR LIBS ###
#############################

if test "$FOAM_FATAL" = yes ; then
	AC_MSG_ERROR([Cannot build FOAM! Missing libraries:$FOAM_MISSING!])
fi

AC_MSG_NOTICE([===============Summary================])


AC_MSG_NOTICE([---------------Hardware---------------])

if test "$have_dc1394" = true ; then
	AC_MSG_NOTICE([1394 cameras:            supported])
else
	AC_MSG_NOTICE([1394 cameras:            not supported, missing:$hw1394_missing])
fi

if test "$have_andor" = true ; then
	AC_MSG_NOTICE([Andor cameras:           supported])
else
	AC_MSG_NOTICE([Andor cameras:           not supported, missing:$hwandor_missing])
fi

AC_MSG_NOTICE([---------------Targets----------------])

if test "$dummy" = true ; then
	AC_MSG_NOTICE([Dummy mode:              available])
else
	AC_MSG_NOTICE([Dummy mode               disabled])
fi

if test "$simstat" = true ; then
	AC_MSG_NOTICE([Static sim mode:         available])
elif test "$simstat" = disabled; then
	AC_MSG_NOTICE([Static sim mode:         disabled])
else
	AC_MSG_NOTICE([Static sim mode:         not available])
	AC_MSG_NOTICE([Static sim mode:         missing:$simstatmissing])
fi

if test "$fullsim" = true ; then
	AC_MSG_NOTICE([Full sim mode:           available])
elif test "$fullsim" = disabled; then
	AC_MSG_NOTICE([Full sim mode:           disabled])
else
	AC_MSG_NOTICE([Full sim mode:           not available])
	AC_MSG_NOTICE([Full sim mode:           missing:$fullsimmissing])
fi

if test "$gui" = true ; then
	AC_MSG_NOTICE([FOAM control GUI:        available])
elif test "$gui" = disabled; then
	AC_MSG_NOTICE([FOAM control GUI:        disabled])
else
	AC_MSG_NOTICE([FOAM control GUI:        not available])
	AC_MSG_NOTICE([FOAM control GUI:        missing:$guimissing])
fi

#if test "$opengl" = true ; then
#	AC_MSG_NOTICE([OpenGL renderer:         available])
#elif test "$opengl" = disabled; then
#	AC_MSG_NOTICE([OpenGL renderer:         disabled])
#else
#	AC_MSG_NOTICE([OpenGL renderer:         not available])
#	AC_MSG_NOTICE([OpenGL renderer:         missing:$openglmissing])
#fi

if test "$debug" = true ; then
	AC_MSG_NOTICE([Debug targets:           available])
else
	AC_MSG_NOTICE([Debug targets:           disabled])
fi

if test "$hwdebug" = true ; then
	AC_MSG_NOTICE([Hardware debug targets:  available])
else
	AC_MSG_NOTICE([Hardware debug targets:  disabled])
fi

if test "$docs" = true ; then
	AC_MSG_NOTICE([FOAM docs:               available])
elif test "$simstat" = disabled; then
	AC_MSG_NOTICE([FOAM docs:               disabled])
else
	AC_MSG_NOTICE([FOAM docs:               not available])
	AC_MSG_NOTICE([FOAM docs:               missing:$docsmissing])
fi

AC_MSG_NOTICE([======================================])


AC_OUTPUT

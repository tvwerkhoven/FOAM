## @file configure.ac
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
## Copyright 2007-2010 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

AC_INIT([FOAM], [0.1], [t.i.m.vanwerkhoven@xs4all.nl])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([FOAM], [0.1], [t.i.m.vanwerkhoven@xs4all.nl])
AC_PREREQ(2.59)
AC_COPYRIGHT([Copyright (C) 2007-2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>])
AC_REVISION(`git describe`)

AC_CONFIG_SRCDIR(foam/foam.cc)
AC_CONFIG_HEADERS(foam/autoconfig.h)

AC_CONFIG_FILES([
Makefile
foam/Makefile
ui/Makefile
lib/Makefile
conf/Makefile
data/Makefile
testing/Makefile
])

AC_GNU_SOURCE

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
FOAMDATADIR=$datadir/foam
FOAMCONFDIR=$sysconfdir/foam
AC_SUBST(FOAMDATADIR)
AC_SUBST(FOAMCONFDIR)

### These libraries are crucial for all targets
AC_SEARCH_LIBS([sin],
		[m],
		[],
		[FOAM_FATAL=yes;FOAM_MISSING+=" libm"])

AC_SEARCH_LIBS([pthread_create],
		[pthread],
		[],
		[FOAM_FATAL=yes;FOAM_MISSING+=" pthread"])

PKG_CHECK_MODULES(SIGC, [sigc++-2.0 >= 2.0], [],
		[FOAM_FATAL=yes;FOAM_MISSING+=" sigc++-2.0"])

PKG_CHECK_MODULES(GSL, [gsl], [],
		[FOAM_FATAL=yes;FOAM_MISSING+=" gsl gslcblas"])

COMMON_CFLAGS="$SIGC_CFLAGS $GSL_CFLAGS"
COMMON_LIBS="$SIGC_LIBS $GSL_LIBS"
AC_SUBST(COMMON_CFLAGS)
AC_SUBST(COMMON_LIBS)

### Miscellaneous optional libraries

# cfitsio
AC_SEARCH_LIBS([ffopen], [cfitsio], 
		[have_fitsio=true],
		[have_fitsio=false])
AM_CONDITIONAL(HAVE_FITSIO, test x$have_fitsio = xtrue)

### Hardware libraries

# dc1394
PKG_CHECK_MODULES(DC1394, [libdc1394-2],
		[have_dc1394=true],
		[have_dc1394=false])

# TODO: need to check things for linux and mac here
if test "$have_dc1394" = true ; then
	case "$build" in
		*-*-linux*)
		# TODO: test this, do we need it?
		AC_SEARCH_LIBS(raw1394_channel_modify, raw1394,
				[have_dc1394=true],
				[have_dc1394=false;AC_MSG_WARN(libraw1394 not found or too old (<1.2.0), required for libdc1394.)])
		;;
		*-*-darwin*)
		AC_SEARCH_LIBS(IOMasterPort, IOKit,
				[have_dc1394=true],
				[have_dc1394=true;AC_MSG_WARN(IOKitLib not found, required for libdc1394.)])
		;;
	esac
fi
AM_CONDITIONAL(HAVE_DC1394, test x$have_dc1394 = xtrue)


### BUILD TARGETS ###
#####################

dummy=true
AC_ARG_ENABLE([dummy],
		AC_HELP_STRING([--disable-dummy], [build dummy mode]),
		[test "$enableval" = no && dummy=disabled], [])

docs=true
AC_ARG_ENABLE([docs],
		AC_HELP_STRING([--disable-docs], [compile documentation]),
		[test "$enableval" = no && docs=disabled], [])

simstat=true
AC_ARG_ENABLE([simstat],
		AC_HELP_STRING([--disable-simstat], [build static simulation mode]),
		[test "$enableval" = no && simstat=disabled], [])

fullsim=true
AC_ARG_ENABLE([fullsim],
		AC_HELP_STRING([--disable-fullsim], [build full simulation mode]),
		[test "$enableval" = no && fullsim=disabled], [])

debug=true
AC_ARG_ENABLE([debug],
		AC_HELP_STRING([--disable-debug], [disable debugging mode]),
		[test "$enableval" = no && debug=disabled], [])

gui=true
AC_ARG_ENABLE([gui],
		AC_HELP_STRING([--disable-gui], [build gui]),
		[test "$enableval" = no && gui=disabled], [])

AC_ARG_ENABLE([hwdebug],
		AC_HELP_STRING([--enable-hwdebug], [build hardware debugging tests]),
		[test "$enableval" = yes && hwdebug=true], [hwdebug=disabled])

### BUILD OPTIONS ###
#####################
		
AC_ARG_WITH([libsiu],
		AC_HELP_STRING([--with-libsiu], [location of libsiu (def: ./libsiu)]),
		[test "$withval" != false && LIBSIU=$withval],
		[LIBSIU="libsiu"])
		
# @todo test LIBSIU presence
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

### STATIC SIMULATION MODE LIBS ###
###################################
if test "$simstat" = true ; then
	AC_MSG_NOTICE([*** Static simulation enabled, doesn't need extra libs])
	simstatmissing=""
		
	SIMSTAT_CFLAGS=""
	SIMSTAT_LIBS=""
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
	
#	AC_SEARCH_LIBS([deflate], [z],
#		[have_z=true],
#		[have_z=false])
#
#	AC_SEARCH_LIBS([IcsOpen], [ics],
#			[have_ics=true],
#			[have_ics=false])
#
#	if test "$have_z" = true -a "$have_ics" = true; then
#		have_ics=true
#	else
#		fullsim=false
#		fullsimmissing+=" libics"
#	fi

	PKG_CHECK_MODULES(FFTW, fftw3 > 3.2.0, [],
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
	
	PKG_CHECK_MODULES(GTHREAD, gthread-2.0, [],
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
		AC_MSG_ERROR([OpenGL not found.]);
	fi

	PKG_CHECK_MODULES(GLEXTMM, gtkglextmm-1.2, [],
		[AC_MSG_WARN([OpenGL needs gtkglextmm-1.2.]);gui=false;guimissing+=" gtkglextmm-1.2"])

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
	AC_MSG_ERROR([Cannot build FOAM! Missing libraries$FOAM_MISSING!])
fi

AC_MSG_NOTICE([===============Summary================])

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

AC_MSG_NOTICE([======================================])


AC_OUTPUT

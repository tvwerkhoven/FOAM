## @file configure.ac
## @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
## Copyright 2007-2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

AC_INIT([FOAM], [0.1], [t.i.m.vanwerkhoven@xs4all.nl])
AC_COPYRIGHT([Copyright (C) 2007-2009 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>])
AC_REVISION(`git describe`)

AC_CONFIG_HEADERS([foam/autoconfig.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([
Makefile
lib/Makefile
foam/Makefile
conf/Makefile
data/Makefile
testing/Makefile
])

# These are the configuration- and data-directories
FOAMCONFDIR=$datadir/foam/
FOAMDATADIR=./
AC_SUBST(FOAMCONFDIR)
AC_SUBST(FOAMDATADIR)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB

# These libraries are crucial for all targets
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

### BUILD TARGETS ###
#####################

AC_ARG_ENABLE([dummy],
		AC_HELP_STRING([--disable-dummy], [build dummy mode]),
		[test "$enableval" = yes && dummy=disabled], [dummy=true])

AC_ARG_ENABLE([docs],
		AC_HELP_STRING([--disable-docs], [compile documentation]),
		[test "$enableval" = yes && docs=disabled], [docs=true])

AC_ARG_ENABLE([simstat],
		AC_HELP_STRING([--disable-simstat], [build static simulation mode]),
		[test "$enableval" = yes && simstat=disabled], [simstat=true])

AC_ARG_ENABLE([debug],
		AC_HELP_STRING([--enable-debug], [enable debugging mode]),
		[test "$enableval" = yes && debug=true], [debug=disabled])

AC_ARG_ENABLE([hwdebug],
		AC_HELP_STRING([--enable-hwdebug], [build hardware debugging tests]),
		[test "$enableval" = yes && hwdebug=true], [hwdebug=disabled])

### BUILD OPTIONS ###
#####################

AC_ARG_WITH([opengl],
		AC_HELP_STRING([--with-opengl], [use OpenGL instead of SDL]),
		[test "$withval" = yes && opengl=true])
		
AC_ARG_WITH([libsiu],
		AC_HELP_STRING([--with-libsiu], [location of libsiu (def: ./libsiu)]),
		[test "$withval" != false && LIBSIU=$withval],
		[LIBSIU="libsiu"])

### Set some automake variables ###
###################################

AM_CONDITIONAL([DEBUG], [test "$debug" = true])
AM_CONDITIONAL([HWDEBUG], [test "$hwdebug" = true])
AC_SUBST(LIBSIU)

### STATIC SIMULATION MODE ###
##############################
if test "$simstat" = true ; then
	AC_MSG_NOTICE([*** Static simulation enabled, checking for extra libs])
	simstatmissing=""
	
	# Arbitrary version
	SDL_VERSION=1.2.0
	AM_PATH_SDL($SDL_VERSION,
		[],
		AC_MSG_WARN([This package needs SDL in simstat mode]))
	
	PKG_CHECK_MODULES(CFITSIO, [cfitsio], [],
		[AC_MSG_WARN([This package needs cfitsio in simstat mode.]);
		simstat=false;
		simstatmissing+=" cfitsio"])
	
	AC_SEARCH_LIBS([IMG_Load], [SDL_image], [SIMSTAT_LIBS+="-lSDL_image "], 
		[AC_MSG_WARN([This package needs SDL_image in simstat mode.]);
		simstat=false;
		simstatmissing+=" sdl-image"])
	
	AC_SEARCH_LIBS([gdImageCreate], [gd], [SIMSTAT_LIBS+="-lgd "], 
		[AC_MSG_WARN([This package needs gd in simstat mode.]);
		simstat=false;
		simstatmissing+=" gd"])
	
	AC_SEARCH_LIBS([gdImagePng], [png], [SIMSTAT_LIBS+="-lpng "], 
		[AC_MSG_WARN([This package needs png in simstat mode.]);
		simstat=false;
		simstatmissing+=" png"])
	
	SIMSTAT_CFLAGS+=" $SDL_CFLAGS $CFITSIO_CFLAGS"
	SIMSTAT_LIBS+=" $SDL_LIBS $CFITSIO_LIBS"
else 
	AC_MSG_NOTICE([*** Static simulation disabled])
fi

AC_SUBST(SIMSTAT_CFLAGS)
AC_SUBST(SIMSTAT_LIBS)
AM_CONDITIONAL([SIMSTAT], [test "$simstat" = true])

### OPENGL FEATURE ### 
######################
if test "$opengl" = true ; then
	AC_MSG_NOTICE([*** OpenGL enabled, checking for extra libs.])
	AX_CHECK_GL
	AX_CHECK_GLU
	AX_CHECK_GLUT
	
	OPENGL_CFLAGS+="$GL_CFLAGS $GLU_CFLAGS $GLUT_CFLAGS"
	OPENGL_LIBS+="$GL_LIBS $GLU_LIBS $GLUT_LIBS"
	if test "$no_gl" = yes || test  "$no_glu" = yes || test "$no_glut" = yes ; then
		opengl=false
		AC_MSG_WARN([*** OpenGL not found!])
	fi
else 
	AC_MSG_NOTICE([*** OpenGL disabled.])
fi

AC_SUBST(OPENGL_LIBS)
AC_SUBST(OPENGL_CFLAGS)
AM_CONDITIONAL([OPENGL], [test "$opengl" = true])

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

if test "$hwdebug" = true ; then
	AC_MSG_NOTICE([Hardware debug mode:     available])
else
	AC_MSG_NOTICE([Hardware debug mode      disabled])
fi

AC_MSG_NOTICE([======================================])


AC_OUTPUT

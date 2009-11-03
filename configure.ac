## Copyright 2007-2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

AC_INIT([FOAM], [0.1], [t.i.m.vanwerkhoven@xs4all.nl])
AC_COPYRIGHT([Copyright (C) 2007-2009 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>])
AC_REVISION(`git describe`)

AC_CONFIG_HEADERS([lib/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([
Makefile
lib/Makefile
foam/Makefile
conf/Makefile
testing/Makefile
])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB

# These are the configuration- and data-directories
# TODO: necessary?
FOAMCONFDIR=$datadir/conf/
FOAMDATADIR=./
AC_SUBST(FOAMCONFDIR)
AC_SUBST(FOAMDATADIR)

# These libraries are crucial for all targets
AC_SEARCH_LIBS([sin],
		[m],
		[],
		[AC_MSG_ERROR([cannot continue without libm.])])

AC_SEARCH_LIBS([pthread_create],
		[pthread],
		[],
		[AC_MSG_ERROR([cannot continue without pthread.])])

AC_SEARCH_LIBS([bufferevent_base_set], 
		[event],
		[],
		[AC_MSG_ERROR([cannot continue without libevent.])])

AC_SEARCH_LIBS([gsl_blas_dgemm],
		[gsl],
		[],
		[AC_MSG_ERROR([cannot continue without gsl.])])

AC_SEARCH_LIBS([cblas_dgemm],
		[gslcblas],
		[COMMON_LIBS+="-lgslcblas"],
		[AC_MSG_ERROR([cannot continue without gslcblas.])])

### BUILD TARGETS ###
#####################

AC_ARG_ENABLE([dummy],
		AC_HELP_STRING([--disable-dummy], [build dummy mode]),
		[test "$enableval" = yes && dummy=disabled], [dummy=true])

AC_ARG_ENABLE([docs],
		AC_HELP_STRING([--disable-docs], [compile documentation]),
		[test "$enableval" = yes && docs=disabled], [docs=true])

AC_ARG_ENABLE([simstat],
		AC_HELP_STRING([--enable-simstat], [build static simulation mode]),
		[test "$enableval" = yes && simstat=true], [simstat=disabled])

# AC_ARG_ENABLE([simdyn],
# 		AC_HELP_STRING([--enable-simdyn], [build dynamical simulation mode]),
# 		[test "$enableval" = yes && simdyn=true], [simdyn=disabled])

AC_ARG_ENABLE([debug],
		AC_HELP_STRING([--enable-debug], [enable debugging mode]),
		[test "$enableval" = yes && debug=true], [debug=disabled])

# AC_ARG_ENABLE([simhw],
# 		AC_HELP_STRING([--enable-simhw], [build simulation of hardware]),
# 		[test "$enableval" = yes && simhw=true], [simhw=disabled])

AC_ARG_ENABLE([hwdebug],
		AC_HELP_STRING([--enable-hwdebug], [build hardware debugging tests]),
		[test "$enableval" = yes && hwdebug=true], [hwdebug=disabled])

# AC_ARG_ENABLE([mcmath],
# 		AC_HELP_STRING([--enable-mcmath], [build mcmath mode]),
# 		[test "$enableval" = yes && mcmath=true], [mcmath=disabled])

# AC_ARG_ENABLE([ui],
# 		AC_HELP_STRING([--enable-ui], [build the UI]),
# 		[test "$enableval" = yes && ui=true], [ui=disabled])

### BUILD OPTIONS ###
#####################

AC_ARG_WITH([opengl],
		AC_HELP_STRING([--with-opengl], [use OpenGL instead of SDL]),
		[test "$withval" = yes && opengl=true])

### Set some automake variables ###
###################################

AM_CONDITIONAL([DEBUG], [test "$debug" = true])
AM_CONDITIONAL([HWDEBUG], [test "$hwdebug" = true])

### STATIC SIMULATION MODE ###
##############################
if test "$simstat" = true ; then
	AC_MSG_NOTICE([static simulation enabled, checking for extra libs])
	simstatmissing=""
	PKG_CHECK_MODULES(SDL, sdl, [],
		[AC_MSG_WARN([This package needs SDL in simstat mode.])])

	AC_SEARCH_LIBS([IMG_Load], [SDL_image], [SIMSTAT_LIBS+=" -lSDL_image"], 
		[AC_MSG_WARN([This package needs SDL_image in simstat mode.]);
		simstat=false;
		simstatmissing+=" sdl-image"])
	
	AC_SEARCH_LIBS([gdImageCreate], [gd], [SIMSTAT_LIBS+=" -lgd"], 
		[AC_MSG_WARN([This package needs gd in simstat mode.]);
		simstat=false;
		simstatmissing+=" gd"])
	
	AC_SEARCH_LIBS([gdImagePng], [png], [SIMSTAT_LIBS+=" -lpng"], 
		[AC_MSG_WARN([This package needs png in simstat mode.]);
		simstat=false;
		simstatmissing+=" png"])
	
	SIMSTAT_CFLAGS+="$SDL_CFLAGS"
	SIMSTAT_LIBS+="$SDL_LIBS"
else 
	AC_MSG_NOTICE([static simulation disabled])
fi

AC_SUBST(SIMSTAT_CFLAGS)
AC_SUBST(SIMSTAT_LIBS)
AM_CONDITIONAL([SIMSTAT], [test "$simstat" = true])

### DYNAMICAL SIMULATION MODE ###
#################################
# if test "$simdyn" = true ; then
# 	AC_MSG_NOTICE([dynamics simulation enabled, checking for extra libs])
# 	simdynmissing=""
# 	LIBS=""
# 
# 	PKG_CHECK_MODULES(SDL, sdl, [],
# 		[AC_MSG_WARN([This package needs SDL in simdyn mode.])])
# 
# 	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
# 		[AC_MSG_WARN([This package needs SDL_image in simdyn mode.]);
# 		simdyn=false;
# 		simdynmissing+=" sdl-image"])
# 	
# 	AC_SEARCH_LIBS([fftw_execute], [fftw3] ,[], 
# 		[AC_MSG_WARN([This package needs fftw3 in simdyn mode.]);
# 		simdyn=false;
# 		simdynmissing+=" fftw3"])
# 	 
# 	AC_SEARCH_LIBS([gdImageCreate], [gd] ,[], 
# 		[AC_MSG_WARN([This package needs gd in simdyn mode.]);
# 		simdyn=false;
# 		simdynmissing+=" gd"])
# 	
# 	AC_SEARCH_LIBS([gdImagePng], [png] ,[], 
# 		[AC_MSG_WARN([This package needs png in simdyn mode.]);
# 		simdyn=false;
# 		simdynmissing+=" png"])
# 
# 	SIMDYN_CFLAGS+="$SDL_CFLAGS"
# 	SIMDYN_LIBS+="$LIBS $SDL_LIBS"
# else 
# 	AC_MSG_NOTICE([dynamical simulation disabled])
# fi
# 
# AC_SUBST(SIMDYN_CFLAGS)
# AC_SUBST(SIMDYN_LIBS)
# AM_CONDITIONAL([SIMDYN], [test "$simdyn" = true])

### MCMATH MODE ### 
###################
# if test "$mcmath" = true ; then
# 	AC_MSG_NOTICE([mcmath enabled, checking for extra libs])
# 	mcmathmissing=""
# 	LIBS=""
# 
# 	PKG_CHECK_MODULES(SDL, sdl, [],
# 		[AC_MSG_WARN([This package needs SDL in mcmath mode.])])
# 
# 	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
# 		[AC_MSG_WARN([This package needs SDL_image in mcmath mode.]);
# 		mcmath=false;
# 		mcmathmissing+=" sdl-image"])
# 	
# 	AC_SEARCH_LIBS([gdImageCreate], [gd] ,[], 
# 		[AC_MSG_WARN([This package needs gd in mcmath mode.]);
# 		mcmath=false;
# 		mcmathmissing+=" gd"])
# 	
# 	AC_SEARCH_LIBS([gdImagePng], [png] ,[], 
# 		[AC_MSG_WARN([This package needs png in mcmath mode.]);
# 		mcmath=false;
# 		mcmathmissing+=" png"])
# 
# 	MCMATH_CFLAGS+="$SDL_CFLAGS"
# 	MCMATH_LIBS+="$LIBS $SDL_LIBS"
# else 
# 	AC_MSG_NOTICE([mcmath disabled])
# fi
# 
# AC_SUBST(MCMATH_LIBS)
# AC_SUBST(MCMATH_CFLAGS)
# AM_CONDITIONAL([MCMATH], [test "$mcmath" = true])

### UI MODE ###
###############
# if test "$ui" = true ; then
# 	AC_MSG_NOTICE([Cheking ui libs])
# 	uimissing=""
# 	LIBS=""
# 	
# 	PKG_CHECK_MODULES(SDL, sdl, [],
# 		[AC_MSG_WARN([This package needs SDL for ui.])])
# 	
#  	UI_CFLAGS+="$SDL_CFLAGS"
# 	UI_LIBS+="$SDL_LIBS"
# else
# 	AC_MSG_NOTICE([ui disabled])
# fi
# 
# AC_SUBST(UI_LIBS)
# AC_SUBST(UI_CFLAGS)
# AM_CONDITIONAL([UI], [test x$ui = xtrue])

### OPENGL FEATURE ### 
######################
if test "$opengl" = true ; then
	AC_MSG_NOTICE([opengl enabled, checking for extra libs])
	AX_CHECK_GL
	AX_CHECK_GLU
	AX_CHECK_GLUT
	
	OPENGL_CFLAGS+="$GL_CFLAGS $GLU_CFLAGS $GLUT_CFLAGS"
	OPENGL_LIBS+="$GL_LIBS $GLU_LIBS $GLUT_LIBS"
	if test "$no_gl" = yes || test  "$no_glu" = yes || test "$no_glut" = yes ; then
		opengl=false
		AC_MSG_WARN([OpenGL not found.])
	fi
else 
	AC_MSG_NOTICE([OpenGL disabled.])
fi

AC_SUBST(OPENGL_LIBS)
AC_SUBST(OPENGL_CFLAGS)
AM_CONDITIONAL([OPENGL], [test "$opengl" = true])

### END CHECKING FOR LIBS ###
#############################

AC_MSG_NOTICE([===============Summary================])

if test "$simstat" = true ; then
	AC_MSG_NOTICE([Static sim mode:         available])
elif test "$simstat" = disabled; then
	AC_MSG_NOTICE([Static sim mode:         disabled])
else
	AC_MSG_NOTICE([Static sim mode:         not available])
	AC_MSG_NOTICE([Static sim mode:         missing: $simstatmissing])
fi

# if test "$simdyn" = true ; then
# 	AC_MSG_NOTICE([Dynamic sim mode:        available])
# elif test "$simdyn" = disabled; then
# 	AC_MSG_NOTICE([Dynamic sim mode:        disabled])
# else
# 	AC_MSG_NOTICE([Dynamic sim mode:        not available])
# 	AC_MSG_NOTICE([Dynamic sim mode:        $simdynmissing])
# fi

# if test "$mcmath" = true ; then
# 	AC_MSG_NOTICE([mcmath mode:             available])
# elif test "$mcmath" = disabled; then
# 	AC_MSG_NOTICE([mcmath mode:             disabled])
# else
# 	AC_MSG_NOTICE([mcmath mode:             not available])
# 	AC_MSG_NOTICE([mcmath mode:             missing: $mcmathmissing])
# fi

if test "$hwdebug" = true ; then
	AC_MSG_NOTICE([hwdebug mode:            available])
elif test "$hwdebug" = disabled; then
	AC_MSG_NOTICE([hwdebug mode:            disabled])
else
	AC_MSG_NOTICE([hwdebug mode:            unknown])
fi

# if test "$ui" = true ; then
# 	AC_MSG_NOTICE([UI:                      available])
# elif test "$ui" = disabled; then
# 	AC_MSG_NOTICE([UI:                      disabled])
# else
# 	AC_MSG_NOTICE([UI:                      not available])
# 	AC_MSG_NOTICE([UI:                      missing: $uimissing])
# fi

AC_MSG_NOTICE([======================================])

# if test "$simhw" = true ; then
# AC_MSG_NOTICE([Are we simulating hardware:       $simhw])
# else
# 	AC_MSG_NOTICE([Are we simulating hardware:         no])
# fi
# 
# AC_MSG_NOTICE([======================================])

AC_OUTPUT

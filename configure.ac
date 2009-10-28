## Copyright 2007-2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
## $Id$

AC_INIT([FOAM], [m4_esyscmd([./version.sh | tr -d '\n'])], [t.i.m.vanwerkhoven@xs4all.nl])
AC_COPYRIGHT([Copyright (C) 2008 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>])

AC_REVISION($Revision: m4_esyscmd([./version.sh | tr -d '\n']) $)

#AC_CANONICAL_SYSTEM


AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([
Makefile
src/Makefile
conf/Makefile
testing/Makefile
])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB

# These are the configuration- and data-directories
FOAMCONFDIR=$datadir/conf/
FOAMDATADIR=./
AC_SUBST(FOAMCONFDIR)
AC_SUBST(FOAMDATADIR)

# We'd like C99 enabled - breaks GSL, some define goes wrong
#AC_PROG_CC_C99
# We'd like const to work
AC_C_CONST

# We always need these libs
AC_SEARCH_LIBS([sin], [m] ,[], AC_MSG_ERROR([This package needs libm.]) ) 
AC_SEARCH_LIBS([bufferevent_base_set], [event] ,[], AC_MSG_ERROR([This package needs libevent.]) ) 
AC_SEARCH_LIBS([pthread_create], [pthread] ,[], AC_MSG_ERROR([This package needs libpthread.]) ) 
AC_SEARCH_LIBS([cblas_dgemm],[gslcblas], [], AC_MSG_ERROR([This package needs a cblas supplied with gsl like atlas]) ) 
AC_SEARCH_LIBS([gsl_blas_dgemm],[gsl], [], AC_MSG_ERROR([This package needs libgsl.]) ) 

# The following is kinda hacky because autoconf needs CFLAGS to include the 
# libraries necessary for test-compiling, such as the ones above. However,
# we use LIBS as well to detect which libraries we need to include certain 
# packages for the different build targets (simstatic, simdyn, mcmath etc).
# To test this, we reset $LIBS each time and thus compilation tests might fail,
# hence we use this hack.

# save common libs (this is kinda ugly)
COMMON_LIBS=$LIBS
AC_SUBST(COMMON_LIBS)
# add common libs to the compilation tests lateron
CFLAGS+=" $LIBS"

### DEVELOPER SECTION ###
#########################

# You can edit this section if you're using this package in some other 
# configuration than we are. See manual for details.

### BEGIN PKG_CONFIG ###
########################
# Check for PKG_CHECK_MODULES first, for OS X
if test x"$PKG_CONFIG_PATH" = x ; then
	AC_MSG_NOTICE([The PKG_CONFIG_PATH variable was not set])
	AC_MSG_NOTICE([You should set it to the directories that contain the .pc files])
	AC_MSG_NOTICE([Try adding the env variables like: ])
	AC_MSG_NOTICE([ACLOCAL_FLAGS "-I /sw/share/aclocal" ])
	AC_MSG_NOTICE([PKG_CONFIG_PATH "/sw/lib/pkgconfig"])
	AC_MSG_ERROR([Abort])
fi

# This fails on OS X
# PKG_CHECK_MODULES(SDL, sdl) 
# This appears to be slightly more portable
SDL_CFLAGS=$(pkg-config sdl --cflags)
SDL_LIBS=$(pkg-config sdl --libs)

### TEST FOR USER INPUT ###
###########################

# Test if we want to compile the dynamical simulation package or not
AC_ARG_ENABLE([simdyn],
		AC_HELP_STRING([--enable-simdyn], [enable dynamical simulation mode (def: yes)]),
		[case "${enableval}" in
		yes) simdyn=true ;;
		no)  simdyn=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-simdyn]) ;;
	esac],[simdyn=true])

# Test if we want to compile the statical simulation package or not
AC_ARG_ENABLE([simstat],
		AC_HELP_STRING([--enable-simstat], [enable static simulation mode (def: yes)]),
		[case "${enableval}" in
		yes) simstat=true ;;
		no)  simstat=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-simstat]) ;;
	esac],[simstat=true])
		
# Test if we want to include the UI or not
AC_ARG_ENABLE([ui],
		AC_HELP_STRING([--enable-ui], [enable the UI (def: no)]),
		[case "${enableval}" in
		yes) ui=true ;;
		no)  ui=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-ui]) ;;
	esac],[ui=disabled])
	
# Test if we want to do hardware debugging (i.e. compile stand alone modules)
AC_ARG_ENABLE([hwdebug],
		AC_HELP_STRING([--enable-hwdebug], [enable hardware debugging (def: no)]),
		[case "${enableval}" in
		yes) hwdebug=true ;;
		no)  hwdebug=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-hwdebug]) ;;
	esac],[hwdebug=disabled])

# test if we are using OpenGL
AC_ARG_ENABLE([opengl],
		AC_HELP_STRING([--enable-opengl], [enable usage of opengl instead of plain SDL (def: yes)]),
		[case "${enableval}" in
		yes) opengl=true ;;
		no)  opengl=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-opengl]) ;;
	esac],[opengl=true])


# test if we need OS X OpenGL (with frameworks, different compiling)
AC_ARG_ENABLE([osxopengl],
		AC_HELP_STRING([--enable-osxopengl], [use opengl framework on OS X (def: no)]),
		[case "${enableval}" in
		yes) osxopengl=true ;;
		no)  osxopengl=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-osxopengl]) ;;
	esac],[osxopengl=disabled])

# Test if we want mcmath enabled 
AC_ARG_ENABLE([mcmath],
		AC_HELP_STRING([--enable-mcmath], [enable mcmath mode (def: no)]),
		[case "${enableval}" in
		yes) mcmath=true ;;
		no)  mcmath=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-mcmath]) ;;
	esac],[mcmath=disabled])
	
# Test if we want to simulate the hardware or not
AC_ARG_ENABLE([simhw],
		AC_HELP_STRING([--enable-simhw], [enable simulation of hardware (def: no)]),
		[case "${enableval}" in
		yes) simhw=true ;;
		no)  simhw=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-simhw]) ;;
	esac],[simhw=disabled])
	

### Set some automake variables ###
###################################

AM_CONDITIONAL([HWDEBUG], [test x$hwdebug = xtrue])
AM_CONDITIONAL([SIMHW], [test x$simhw = xtrue])


### STATIC SIMULATION MODE ###
##############################
if test "$simstat" = true ; then
	# search for static simulation libraries here, exit if not found
	AC_MSG_NOTICE([static simulation enabled, checking for extra libs])
	# this is kinda ugly
	simstatmissing=""
	LIBS=""
	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
		[AC_MSG_WARN([This package needs sdl-image in simstat mode.])
		simstat=false
		simstatmissing+=" sdl-image"
		]
	)
	
	AC_SEARCH_LIBS([gdImageCreate], [gd] ,[], 
		[AC_MSG_WARN([This package needs libgd in simstat mode.])
		simstat=false
		simstatmissing+=" gd"
		]
	)
	
	AC_SEARCH_LIBS([gdImagePng], [png] ,[], 
		[AC_MSG_WARN([This package needs libpng in simstat mode.])
		simstat=false
		simstatmissing+=" png"
		]
	)	
	
	# fix simulation Cflags (mainly SDL)
	SIMSTAT_CFLAGS+="$SDL_CFLAGS"
	SIMSTAT_LIBS+="$LIBS $SDL_LIBS"
else 
	# search for runtime libs here (camera stuff), exit if not found
	AC_MSG_NOTICE([static simulation disabled])
fi
AC_SUBST(SIMSTAT_CFLAGS)
AC_SUBST(SIMSTAT_LIBS)
AM_CONDITIONAL([SIMSTAT], [test x$simstat = xtrue])

### DYNAMICAL SIMULATION MODE ###
#################################
if test "$simdyn" = true ; then
	# search for dynamical simulation libraries here, exit if not found
	AC_MSG_NOTICE([dynamicsl simulation enabled, checking for extra libs])
	# this is kinda ugly
	simdynnmissing=""
	LIBS=""
	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
		[AC_MSG_WARN([This package needs sdl-image in simdyn mode.])
		simdyn=false
		simdynmissing+=" sdl-image"
		]
	)
	
	AC_SEARCH_LIBS([fftw_execute], [fftw3] ,[], 
		[AC_MSG_WARN([This package needs libfftw3 in simdyn mode.])
		simdyn=false
		simdynmissing+=" fftw3"
		]
	) 	
	 
	AC_SEARCH_LIBS([SDL_Init], [SDL] ,[], 
		[AC_MSG_WARN([This package needs libsdl in simdyn mode.])
		simdyn=false
		simdynmissing+=" sdl"
		]
	)
	
	AC_SEARCH_LIBS([gdImageCreate], [gd] ,[], 
		[AC_MSG_WARN([This package needs libgd in simdyn mode.])
		simdyn=false
		simdynmissing+=" gd"
		]
	)
	
	AC_SEARCH_LIBS([gdImagePng], [png] ,[], 
		[AC_MSG_WARN([This package needs libpng in simdyn mode.])
		simdyn=false
		simdynmissing+=" png"
		]
	)	

	# fix simulation Cflags (mainly SDL)
	SIMDYN_CFLAGS+="$SDL_CFLAGS"
	SIMDYN_LIBS+="$LIBS $SDL_LIBS"
else 
	# oops, didn't work out
	AC_MSG_NOTICE([dynamical simulation disabled])
fi
AC_SUBST(SIMDYN_CFLAGS)
AC_SUBST(SIMDYN_LIBS)
AM_CONDITIONAL([SIMDYN], [test x$simdyn = xtrue])


### MCMATH MODE ### 
###################
if test "$mcmath" = true ; then
	# search for mcmath libs here
	mcmathmissing=""
	AC_MSG_NOTICE([mcmath enabled, checking for extra libs])
	LIBS=""

	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
		[AC_MSG_WARN([This package needs sdl-image in mcmath mode.])
		mcmath=false
		mcmathmissing+=" sdl-image"
		]
	)
	
	AC_SEARCH_LIBS([gdImageCreate], [gd] ,[], 
		[AC_MSG_WARN([This package needs libgd in mcmath mode.])
		mcmath=false
		mcmathmissing+=" gd"
		]
	)
	
	AC_SEARCH_LIBS([gdImagePng], [png] ,[], 
		[AC_MSG_WARN([This package needs libpng in mcmath mode.])
		mcmath=false
		mcmathmissing+=" png"
		]
	)	

	# fix mcmath cflags / libs
	MCMATH_CFLAGS+="$SDL_CFLAGS"
	MCMATH_LIBS+="$LIBS $SDL_LIBS"
else 
	# search for runtime libs here (camera stuff), exit if not found
	AC_MSG_NOTICE([mcmath disabled])
fi
AC_SUBST(MCMATH_LIBS)
AC_SUBST(MCMATH_CFLAGS)
AM_CONDITIONAL([MCMATH], [test x$mcmath = xtrue])

### OPENGL FEATURE ### 
######################
if test "$opengl" = true ; then
	AC_MSG_NOTICE([opengl enabled, checking for extra libs])
	if test "$osxopengl" = true ; then
		AC_MSG_NOTICE([opengl: checking for OS X libs])
# OS X specific OpenGL config, this is 'hardcoded' as I don't know how to get
# autotools to work with frameworks used in OS X.
		OPENGL_CFLAGS+="-FOpenGL -FGLUT"
		OPENGL_LIBS+="-framework OpenGL -framework GLUT"
	else
# search for opengl libs here, generic search
		AC_MSG_NOTICE([opengl: checking for generic libs (non OS X)])
		openglmissing=""
		LIBS=""

		AC_SEARCH_LIBS([glClear], [GL] ,[], 
				[AC_MSG_WARN([OpenGL could not be found on this system.])
				opengl=false
				openglmissing+=" libgl"
				]
			      )

		AC_SEARCH_LIBS([gluOrtho2D], [GLU] ,[], 
				[AC_MSG_WARN([libGLU could not be found on this system.])
				opengl=false
				openglmissing+=" libglu"
				]
			      )

		AC_SEARCH_LIBS([glutGet], [glut] ,[], 
				[AC_MSG_WARN([libglut could not be found on this system.])
				opengl=false
				openglmissing+=" libglut"
				]
			      )

# fix opengl cflags / libs
		OPENGL_CFLAGS+=""
		OPENGL_LIBS+="$LIBS"
	fi
else 
	AC_MSG_NOTICE([OpenGL disabled])
fi
AC_SUBST(OPENGL_LIBS)
AC_SUBST(OPENGL_CFLAGS)
AM_CONDITIONAL([OPENGL], [test x$opengl = xtrue])

### UI MODE ###
###############
if test "$ui" = true ; then
	uimissing=""
	# search for ui libs here
	AC_MSG_NOTICE([Cheking UI libs])
	LIBS=""
	AC_SEARCH_LIBS([SDL_Init], [SDL] ,[], 
		[AC_MSG_WARN([This package needs libsdl in ui mode.])
		ui=false
		uimissing+=" sdl"
		]
	)
 
	# fix simulation Cflags (mainly SDL)
	UI_CFLAGS+="$SDL_CFLAGS"
	UI_LIBS+="$LIBS $SDL_LIBS"
else
	# search for runtime libs here (camera stuff), exit if not found
	AC_MSG_NOTICE([ui disabled])
fi
AC_SUBST(UI_LIBS)
AC_SUBST(UI_CFLAGS)

AM_CONDITIONAL([UI], [test x$ui = xtrue])

LIBS=""

### END CHECKING FOR MODES ###
##############################

AC_MSG_NOTICE([===========Summary============])

if test "$simdyn" = true ; then
	AC_MSG_NOTICE([Dyn sim mode:    available])
elif test "$simdyn" = disabled; then
	AC_MSG_NOTICE([Dyn sim mode:    disabled])
else
	AC_MSG_NOTICE([Dyn sim mode:    not available])
	AC_MSG_NOTICE([Dyn sim mode:    $dynsimmissing])
fi

if test "$simstat" = true ; then
	AC_MSG_NOTICE([Static sim mode: available])
elif test "$simstat" = disabled; then
	AC_MSG_NOTICE([Static sim mode: disabled])
else
	AC_MSG_NOTICE([Static sim mode: not available])
	AC_MSG_NOTICE([Static sim mode: missing: $simstatmissing])
fi

if test "$mcmath" = true ; then
	AC_MSG_NOTICE([mcmath mode:     available])
elif test "$mcmath" = disabled; then
	AC_MSG_NOTICE([mcmath mode:     disabled])
else
	AC_MSG_NOTICE([mcmath mode:     not available])
	AC_MSG_NOTICE([mcmath mode:     missing: $mcmathmissing])
fi

if test "$hwdebug" = true ; then
	AC_MSG_NOTICE([hwdebug mode:    available])
elif test "$hwdebug" = disabled; then
	AC_MSG_NOTICE([hwdebug mode:    disabled])
else
	AC_MSG_NOTICE([hwdebug mode:    unknown])
fi

if test "$ui" = true ; then
	AC_MSG_NOTICE([UI:              available])
elif test "$ui" = disabled; then
	AC_MSG_NOTICE([UI:              disabled])
else
	AC_MSG_NOTICE([UI:              not available])
	AC_MSG_NOTICE([UI:              missing: $uimissing])
fi

AC_MSG_NOTICE([==============================])

if test "$simhw" = true ; then
	AC_MSG_NOTICE([Are we simulating hardware: yes])
else
	AC_MSG_NOTICE([Are we simulating hardware: no])
fi

AC_MSG_NOTICE([==============================])

AC_OUTPUT

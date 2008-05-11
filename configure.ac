# dynamic versioning does not work :<
#MAJOR=alpha
#MINOR=$(shell svn log -r HEAD | head -n 2 | tail -n 1 | awk '{print$1"-"$5}')
AC_INIT([FOAM], [alpha-2008-04-02], [T.I.M.vanWerkhoven@phys.uu.nl])
AC_COPYRIGHT([Copyright 2007-2008 Tim van Werkhoven <T.I.M.vanWerkhoven@phys.uu.nl>])

AC_REVISION($Revision: 195 $)

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_FILES([
Makefile
src/Makefile
config/Makefile
testing/Makefile
])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])


AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB


# We'd like C99 enabled - breaks GSL, some define goes wrong
#AC_PROG_CC_C99
# We'd like const to work
AC_C_CONST

# We always need these libs
AC_SEARCH_LIBS([sin], [m] ,[], AC_MSG_ERROR([This package needs libm.]) ) 
AC_SEARCH_LIBS([event_init], [event] ,[], AC_MSG_ERROR([This package needs libevent.]) ) 
AC_SEARCH_LIBS([pthread_create], [pthread] ,[], AC_MSG_ERROR([This package needs libpthread.]) ) 
AC_SEARCH_LIBS([cblas_dgemm],[gslcblas], [], AC_MSG_ERROR([This package needs a cblas supplied with gsl like atlas]) ) 
AC_SEARCH_LIBS([gsl_blas_dgemm],[gsl], [], AC_MSG_ERROR([This package needs libgsl.]) ) 

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
# ToDo: is this syntax ok?
if test x"$PKG_CONFIG_PATH" = x ; then
	AC_MSG_NOTICE([The PKG_CONFIG_PATH variable was not set])
	AC_MSG_NOTICE([You should set it to the directories that contain the .pc files])
	AC_MSG_NOTICE([Try adding the env variables like: ])
	AC_MSG_NOTICE([ACLOCAL_FLAGS "-I /sw/share/aclocal" ])
	AC_MSG_NOTICE([PKG_CONFIG_PATH "/sw/lib/pkgconfig"])
	AC_MSG_ERROR([Abort])
fi

PKG_CHECK_MODULES(SDL, sdl) 
#SDL_CFLAGS = $(pkg-config SDL --cflags),
#SDL_LIBS = $(pkg-config SDL --libs)

### TEST FOR USER INPUT ###
###########################

# Test if we want to run as simulation or not
AC_ARG_ENABLE([simulation],
		AC_HELP_STRING([--enable-simulation], [enable simulation mode (def: no)]),
		[case "${enableval}" in
		yes) simul=true ;;
		no)  simul=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-simulation]) ;;
	esac],[simul=disabled])

# Test if we want to run as simulation or not
AC_ARG_ENABLE([statsimulation],
		AC_HELP_STRING([--enable-statsimulation], [enable static simulation mode (def: no)]),
		[case "${enableval}" in
		yes) statsimul=true ;;
		no)  statsimul=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-statsimulation]) ;;
	esac],[statsimul=disabled])
		
# Test if we want to include the UI or not
AC_ARG_ENABLE([ui],
		AC_HELP_STRING([--enable-ui], [enable the UI (def: yes)]),
		[case "${enableval}" in
		yes) ui=true ;;
		no)  ui=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-ui]) ;;
	esac],[ui=true])
	
# Test if we want to do hardware debugging (stand alone modules)
AC_ARG_ENABLE([hwdebug],
		AC_HELP_STRING([--enable-hwdebug], [enable hardware debugging (def: yes)]),
		[case "${enableval}" in
		yes) hwdebug=true ;;
		no)  hwdebug=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-hwdebug]) ;;
	esac],[hwdebug=true])

# Test if we want mcmath enabled 
AC_ARG_ENABLE([mcmath],
		AC_HELP_STRING([--enable-mcmath], [enable mcmath mode (def: yes)]),
		[case "${enableval}" in
		yes) mcmath=true ;;
		no)  mcmath=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-mcmath]) ;;
	esac],[mcmath=true])

AC_ARG_ENABLE([opengl],
		AC_HELP_STRING([--enable-opengl], [enable usage of opengl instead of plain SDL (def: yes)]),
		[case "${enableval}" in
		yes) opengl=true ;;
		no)  opengl=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-opengl]) ;;
	esac],[opengl=true])

### SIMULATION MODE ###
#######################
if test "$simul" = true ; then
	# search for simulation libraries here, exit if not found
	AC_MSG_NOTICE([simulation enabled, checking for extra libs])
	# this is kinda ugly
	simulmissing=""
	LIBS=""
	AC_SEARCH_LIBS([fftw_execute], [fftw3] ,[], 
		[AC_MSG_WARN([This package needs libfftw3 in simulation mode.])
		simul=false
		simulmissing+=" fftw3"
		]
	) 	
	 
	AC_SEARCH_LIBS([SDL_Init], [SDL] ,[], 
		[AC_MSG_WARN([This package needs libsdl in simulation mode.])
		simul=false
		simulmissing+=" sdl"
		]
	)

	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
		[AC_MSG_WARN([This package needs sdl-image in simulation mode.])
		simul=false
		simulmissing+=" sdl-image"
		]
	)
	
	AC_SEARCH_LIBS([gdImageCreate], [gd] ,[], 
		[AC_MSG_WARN([This package needs libgd in simulation mode.])
		simul=false
		simulmissing+=" gd"
		]
	)
	
	AC_SEARCH_LIBS([gdImagePng], [png] ,[], 
		[AC_MSG_WARN([This package needs libpng in simulation mode.])
		simul=false
		simulmissing+=" png"
		]
	)	
	
	# fix simulation Cflags (mainly SDL)
	SIMULATION_CFLAGS+="$SDL_CFLAGS"
	SIMULATION_LIBS+="$LIBS $SDL_LIBS"
else 
	# search for runtime libs here (camera stuff), exit if not found
	AC_MSG_NOTICE([simulation disabled])
fi
AC_SUBST(SIMULATION_CFLAGS)
AC_SUBST(SIMULATION_LIBS)
AM_CONDITIONAL([SIMULATION], [test x$simul = xtrue])

### STATIC SIMULATION MODE ###
##############################
if test "$statsimul" = true ; then
	# search for static simulation libraries here, exit if not found
	AC_MSG_NOTICE([static simulation enabled, checking for extra libs])
	# this is kinda ugly
	statsimulmissing=""
	LIBS=""
	AC_SEARCH_LIBS([SDL_Init], [SDL] ,[], 
		[AC_MSG_WARN([This package needs libsdl in static simulation mode.])
		statsimul=false
		statsimulmissing+=" sdl"
		]
	)

	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
		[AC_MSG_WARN([This package needs sdl-image in static simulation mode.])
		statsimul=false
		statsimulmissing+=" sdl-image"
		]
	)
	
	AC_SEARCH_LIBS([gdImageCreate], [gd] ,[], 
		[AC_MSG_WARN([This package needs libgd in simulation mode.])
		simul=false
		simulmissing+=" gd"
		]
	)
	
	AC_SEARCH_LIBS([gdImagePng], [png] ,[], 
		[AC_MSG_WARN([This package needs libpng in simulation mode.])
		simul=false
		simulmissing+=" png"
		]
	)	
	
	# fix simulation Cflags (mainly SDL)
	STATSIMULATION_CFLAGS+="$SDL_CFLAGS"
	STATSIMULATION_LIBS+="$LIBS $SDL_LIBS"
else 
	# search for runtime libs here (camera stuff), exit if not found
	AC_MSG_NOTICE([static simulation disabled])
fi
AC_SUBST(STATSIMULATION_CFLAGS)
AC_SUBST(STATSIMULATION_LIBS)
AM_CONDITIONAL([STATSIMULATION], [test x$statsimul = xtrue])

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
	# search for opengl libs here
	openglmissing=""
	AC_MSG_NOTICE([opengl enabled, checking for extra libs])
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

	# fix opengl cflags / libs
	OPENGL_CFLAGS+=""
	OPENGL_LIBS+="$LIBS"
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

AC_MSG_NOTICE([Configuration summary follows:])
AC_MSG_NOTICE([==============================])
if test "$simul" = true ; then
	AC_MSG_NOTICE([Simulation mode: available])
elif test "$simul" = disabled; then
	AC_MSG_NOTICE([Simulation mode: disabled])
else
	AC_MSG_NOTICE([Simulation mode: not available])
	AC_MSG_NOTICE([Simulation mode: missing: $simulmissing])
fi

if test "$statsimul" = true ; then
	AC_MSG_NOTICE([Static sim mode: available])
elif test "$statsimul" = disabled; then
	AC_MSG_NOTICE([Static sim mode: disabled])
else
	AC_MSG_NOTICE([Static sim mode: not available])
	AC_MSG_NOTICE([Static sim mode: missing: $statsimulmissing])
fi

if test "$mcmath" = true ; then
	AC_MSG_NOTICE([mcmath mode:     available])
elif test "$mcmath" = disabled; then
	AC_MSG_NOTICE([mcmath mode:     disabled])
else
	AC_MSG_NOTICE([mcmath mode:     not available])
	AC_MSG_NOTICE([mcmath mode:     missing: $mcmathmissing])
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


AC_OUTPUT

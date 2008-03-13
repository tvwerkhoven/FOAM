AC_INIT([FOAM], [alpha], [T.I.M.vanWerkhoven@phys.uu.nl])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_COPYRIGHT([Copyright 2007-2008 Tim van Werkhoven <T.I.M.vanWerkhoven@phys.uu.nl>])

AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
Makefile
src/Makefile
])

# We always need these libs
AC_SEARCH_LIBS([sin], [m] ,[], AC_MSG_ERROR([This package needs libm.]) ) 
AC_SEARCH_LIBS([event_init], [event] ,[], AC_MSG_ERROR([This package needs libevent.]) ) 
AC_SEARCH_LIBS([pthread_create], [pthread] ,[], AC_MSG_ERROR([This package needs libpthread.]) ) 
AC_SEARCH_LIBS([cblas_dgemm],[gslcblas], [], AC_MSG_ERROR([This package needs a cblas.]) ) 
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
# TvW: meer PKG_CHECK_MODULES, check docs
PKG_CHECK_MODULES(SDL, sdl) # seems to give errors some times
#SDL_CFLAGS = $(pkg-config SDL --cflags)
#SDL_LIBS = $(pkg-config SDL --libs)

### TEST FOR USER INPUT ###
###########################

# Test if we want to run as simulation or not
AC_ARG_ENABLE([simulation],
		AC_HELP_STRING([--enable-simulation], [enable simulation mode])],
		[case "${enableval}" in
		yes) simul=true ;;
		no)  simul=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-simulation]) ;;
	esac],[simul=true])

# Test if we want to run as simulation or not
AC_ARG_ENABLE([statsimulation],
		AC_HELP_STRING([--enable-statsimulation], [enable static simulation mode])],
		[case "${enableval}" in
		yes) statsimul=true ;;
		no)  statsimul=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-statsimulation]) ;;
	esac],[statsimul=true])
		
# Test if we want to include the UI or not
AC_ARG_ENABLE([ui],
		AC_HELP_STRING([--enable-ui], [enable the UI]),
		[case "${enableval}" in
		yes) ui=true ;;
		no)  ui=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-ui]) ;;
	esac],[ui=true])

# Test if we want ITIFG enabled (default no, unfinished stuff)
AC_ARG_ENABLE([itifg],
		AC_HELP_STRING([--enable-itifg], [enable itifg mode]),
		[case "${enableval}" in
		yes) itifg=true ;;
		no)  itifg=disabled ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-itifg]) ;;
	esac],[itifg=disabled])

### SIMULATION MODE ###
#######################
if test "$simul" = true ; then
	# search for simulation libraries here, exit if not found
	AC_MSG_NOTICE([simulation enabled, checking for extra libs])
	# this is kinda ugly
	simulmissing=""
	LIBS=""
	AC_SEARCH_LIBS([fftw_execute], [fftw3] ,[], 
		[AC_MSG_WARN([This package needs libfftw3 in simulation mode, disabling simulation.])
		simul=false
		simulmissing+=" fftw3"
		]
	) 	
	 
	AC_SEARCH_LIBS([SDL_Init], [SDL] ,[], 
		[AC_MSG_WARN([This package needs libsdl in simulation mode, disabling simulation.])
		simul=false
		simulmissing+=" sdl"
		]
	)

	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
		[AC_MSG_WARN([This package needs sdl-image in simulation mode, disabling simulation.])
		simul=false
		simulmissing+=" sdl-image"
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
		[AC_MSG_WARN([This package needs libsdl in static simulation mode, disabling static simulation.])
		statsimul=false
		statsimulmissing+=" sdl"
		]
	)

	AC_SEARCH_LIBS([IMG_Load], [SDL_image] ,[], 
		[AC_MSG_WARN([This package needs sdl-image in static simulation mode, disabling static simulation.])
		statsimul=false
		statsimulmissing+=" sdl-image"
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

### ITIFG MODE ### 
##################
if test "$itifg" = true ; then
	# search for itifg libs here
	itifgmissing=""
	AC_MSG_NOTICE([itifg enabled, checking for extra libs])
	LIBS=""
else 
	# search for runtime libs here (camera stuff), exit if not found
	AC_MSG_NOTICE([itifg disabled])
fi
AC_SUBST(ITIFG_LIBS)
AC_SUBST(ITIFG_CFLAGS)
AM_CONDITIONAL([ITIFG], [test x$itifg = xtrue])

### UI MODE ###
###############
if test "$ui" = true ; then
	uimissing=""
	# search for itifg libs here
	AC_MSG_NOTICE([Cheking UI libs])
	LIBS=""
	AC_SEARCH_LIBS([SDL_Init], [sdl] ,[], 
		[AC_MSG_WARN([This package needs libsdl in ui mode, disabling simulation.])
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
# TvW: errors 
	AC_MSG_ERROR([Simulation mode: not available])
	AC_MSG_ERROR([Simulation mode: missing: $simulmissing])
fi

if test "$statsimul" = true ; then
	AC_MSG_NOTICE([Static sim mode: available])
elif test "$statsimul" = disabled; then
	AC_MSG_NOTICE([Static sim mode: disabled])
else
# TvW: errors 
	AC_MSG_ERROR([Static sim mode: not available])
	AC_MSG_ERROR([Static sim mode: missing: $statsimulmissing])
fi

if test "$ui" = true ; then
	AC_MSG_NOTICE([UI:              available])
elif test "$ui" = disabled; then
	AC_MSG_NOTICE([UI:              disabled])
else
	AC_MSG_ERROR([UI:              not available])
	AC_MSG_ERROR([UI:              missing: $uimissing])
fi

if test "$itifg" = true ; then
	AC_MSG_NOTICE([ITIFG mode:      available])
elif test "$itifg" = disabled; then
	AC_MSG_NOTICE([ITIFG mode:      disabled])
else
	AC_MSG_ERROR([ITIFG mode:      not available])
	AC_MSG_ERROR([ITIFG mode:      missing: $itifgmissing])
fi

AC_MSG_NOTICE([==============================])


AC_OUTPUT

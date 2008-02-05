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
AC_SEARCH_LIBS([event_init], [event] ,[], AC_MSG_ERROR([This package needs libevent.]) ) 
AC_SEARCH_LIBS([pthread_create], [pthread] ,[], AC_MSG_ERROR([This package needs libpthread.]) ) 

# save common libs (this is kinda ugly)
COMMON_LIBS=$LIBS
AC_SUBST(COMMON_LIBS)

### DEVELOPER SECTION ###
#########################

# You can edit this section if you're using this package in some other 
# configuration than we are. See manual for details.

### BEGIN PKG_CONFIG ###
########################
PKG_CHECK_MODULES(SDL, sdl)

# Test if we want to run as simulation or not
AC_ARG_ENABLE([simulation],
		AC_HELP_STRING([--enable-simulation], [enable simulation mode])],
		[case "${enableval}" in
		yes) simul=true ;;
		no)  simul=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-simulation]) ;;
	esac],[simul=true])

# Test if we want to include the UI or not
AC_ARG_ENABLE([ui],
		AC_HELP_STRING([--enable-ui], [enable the UI]),
		[case "${enableval}" in
		yes) ui=true ;;
		no)  ui=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-ui]) ;;
	esac],[ui=true])

# Test if we want ITIFG enabled 
AC_ARG_ENABLE([itifg],
		AC_HELP_STRING([--enable-itifg], [enable itifg mode]),
		[case "${enableval}" in
		yes) itifg=true ;;
		no)  itifg=false ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-itifg]) ;;
	esac],[itifg=true])

### SIMULATION MODE ###
#######################
if test "$simul" = true ; then
	# search for simulation libraries here, exit if not found
	AC_MSG_NOTICE([simulation enabled, checking for extra libs])
	# this is kinda ugly
	LIBS=""
	AC_SEARCH_LIBS([ffopen], [cfitsio] ,[], 
		[AC_MSG_WARN([This package needs libfitsio in simulation mode, disabling simulation.])]
		simul=false
	) 
	AC_SEARCH_LIBS([fftw_execute], [fftw3] ,[], 
		[AC_MSG_WARN([This package needs libfftw3 in simulation mode, disabling simulation.])
		simul=false]
	) 
	AC_SEARCH_LIBS([SDL_Init], [sdl] ,[], 
		[AC_MSG_WARN([This package needs libsdl in simulation mode, disabling simulation.])
		simul=false]
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

### ITIFG MODE ### 
##################
if test "$itifg" = true ; then
	# search for itifg libs here
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
	# search for itifg libs here
	AC_MSG_NOTICE([Cheking UI libs])
	LIBS=""
	AC_SEARCH_LIBS([SDL_Init], [sdl] ,[], 
		[AC_MSG_WARN([This package needs libsdl in ui mode, disabling simulation.])
		ui=false]
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


AC_OUTPUT

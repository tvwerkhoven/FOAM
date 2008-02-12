/*! @file foam_library.h 
@brief This file is the main library for things shared between all @name components.

(was ao_library.h )

This header files includes most libraries that are used in the UI and the CS components of @name.
It also contains some function declarations used in the UI and CS. See @cslib and
@uilib for the specific CS and UI header files.\n
Last: 2008-01-21
*/

#ifndef FOAM_LIBRARY
#define FOAM_LIBRARY

// HEADER FILES //
/****************/

// TvW: TODO bufferend events zonder LINE_MAX
#ifndef LINE_MAX
#define LINE_MAX 2048
#endif


#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>		// we need math
#include <sys/socket.h>	// networking
#include <arpa/inet.h>	// networking
#include <sys/types.h>
#include <sys/errno.h>
//#include <sys/select.h> //?
#include <stdlib.h>
#include <syslog.h> 	// used for syslogging
#include <stdarg.h>
#include <pthread.h> 	// threads
#include <limits.h> 	// LINE_MAX
#include <stdbool.h> 	// true/false
#include <signal.h> 	// signal handlers
#include <time.h> 		// needed by libevent/event.h
typedef unsigned char u_char;
#include <event.h> 		// include AFTER stdlib.h (which defined u_char needed by event.h)
//#include <sys/uio.h> //?
//#include <inttypes.h> //?
//#include <xlocale.h> //?


// STRUCTS ETC //
/***************/

/*!
@brief Helper enum for loglevel. Can be LOGNONE, LOGERR, LOGINFO or LOGDEBUG.
*/
typedef enum { // level_t
	LOGNONE, LOGERR, LOGINFO, LOGDEBUG
} level_t;


// PROTOTYPES //
/**************/

/*!
@brief Writes the current UTC datetime to *ret, and a pointer to *ret in **ret.

@param [out] **ret A pointer to a date-time string (*ret) is stored in here.
@return EXIT_SUCCESS upon success.
*/
int printUTCDateTime(char **ret);


/*! 
@brief Send data over a socket

sendMsg() sends data to the server, but is currently simply a wrapper for write()

@param [in] sock the socket to send over
@param [in] *buf the string to send
@return same as write(), number of bytes or -1 on error
*/
int sendMsg(const int sock, const char *buf);

#endif /* FOAM_LIBRARY */

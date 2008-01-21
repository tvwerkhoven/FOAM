/*! @file ao_library.h 
@brief This file is the main library for things shared between all FOAM components.

This header files includes most libraries that are used in the UI and the CS components of FOAM.
It also contains some function declarations used in the UI and CS. See \a cs_library.h and
\a ui_library.h for the specific CS and UI header files.\n
Last: 2008-01-21
*/

#ifndef AO_LIBRARY
#define AO_LIBRARY

// HEADER FILES //
/****************/


#ifdef __linux__ 
#include <bits/posix2_lim.h> /* we need this for LINE_MAX? */
#endif

#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>
#include <limits.h>
#include <stdbool.h>
// This is a hack, u_char needed by event.h but not defined on all systems. TODO
typedef unsigned char u_char;
#include <event.h>
#include <sys/uio.h> //?
#include <inttypes.h> //?
#include <time.h>
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
@brief Read data from a socket, or close it if \c EOF.

Read data from a socket into the char array \a msg pointed to
by \a *msg. If \c EOF is received, remove the socket from \a lfd_set
and if an error occured, return \c EXIT_FAILURE.
@param [in] sock Socket with pending data
@param [out] *msg A char array to store the message in
@param [in,out] *lfd_set A pointer to the set of FD's to remove sockets from upon disconnects
@return Number of received bytes if successful, 0 if \c EOF received, \c EXIT_FAILURE otherwise.
*/
//int sockRead(const int sock, char *msg, fd_set *lfd_set);

/*!
@brief Writes the current UTC datetime to *ret, and a pointer to *ret in **ret.

@param [out] **ret A pointer to a date-time string (*ret) is stored in here.
@return EXIT_SUCCESS upon success.
*/
int printUTCDateTime(char **ret);


/*! 
@brief Send data over a socket

@param [in] sock the socket to send over
@param [in] *buf the string to send
@param [in] len the length of the string to send
@return same as write(), number of bytes or -1 on error

sendMsg() sends data to the server, but is currently simply a wrapper for write()
*/
int sendMsg(const int sock, const char *buf);

#endif /* AO_LIBRARY */


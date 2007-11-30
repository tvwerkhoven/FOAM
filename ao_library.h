/*! @file ao_library.h 
@brief short desc for file ao_lib.h
longer desc
*/

#ifndef AO_LIBRARY
#define AO_LIBRARY

// HEADER FILES //
/****************/


#include <string.h>
#include <unistd.h>
#ifdef linux /* we need this for usleep?? */
#define _XOPEN_SOURCE 500
#endif

#ifdef linux /* we need this for LINE_MAX? */
#include <bits/posix2_lim.h>
#endif

#include <sys/time.h>
#include <stdio.h>
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
//#include <event.h>
#include <sys/uio.h> //?
#include <inttypes.h> //?
#include <xlocale.h> //?


// STRUCTS ETC //
/***************/

/*!
@brief Helper struct for logleve used by various structs
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
@param [in] msglen The length of the message
@param [in,out] *lfd_set A pointer to the set of FD's to remove sockets from upon disconnects
@return Number of received bytes if successful, 0 if \c EOF received, \c EXIT_FAILURE otherwise.
*/
int sockRead(int sock, char *msg, size_t msglen, fd_set *lfd_set);

#endif /* AO_LIBRARY */

/*! 
 @file foam_ui_library.h
 @author @authortim
 @date November 14 2007
 
 @brief This is the header file for the connecting client
 
 At the moment this is not more than a collection of lines of code, it might
 not work (very well), use telnet instead.
 */

#ifndef FOAM_UI_LIBRARY
#define FOAM_UI_LIBRARY

// INCLUDES //
/************/

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
//#include <limits.h> 	// LINE_MAX
#include <stdbool.h> 	// true/false
#include <signal.h> 	// signal handlers
#include <time.h> 		// needed by libevent/event.h
typedef unsigned char u_char;
#include <event.h> 		// include AFTER stdlib.h (which defined u_char needed by event.h)

// STRUCTS ETC //
/***************/

/*!
@brief Helper enum for loglevel. Can be LOGNONE, LOGERR, LOGINFO or LOGDEBUG.
*/
typedef enum { // level_t
	LOGNONE, LOGERR, LOGINFO, LOGDEBUG
} level_t;

typedef struct {
	level_t loglevel;
	bool use_syslog;
} config_t;

// PROTOTYPES //
/**************/


/*! 
@brief Send data over a socket

sendMsg() sends data to the server, but is currently simply a wrapper for write()

@param [in] sock the socket to send over
@param [in] *buf the string to send
@return same as write(), number of bytes or -1 on error
*/
int sendMsg(const int sock, const char *buf);

int initSockC(in_addr_t host, int port, fd_set *cfd_set);
int parseArgs(int argc, char *argv[], in_addr_t *host, int *port);
int sockGetActive(fd_set *cfd_set);
void logDebug(const char *msg, ...);
void logErr(const char *msg, ...);
void logInfo(const char *msg, ...);
int sockRead(const int sock, char *msg, fd_set *lfd_set);

// DEFINES //
/***********/

#endif /* FOAM_UI_LIBRARY */

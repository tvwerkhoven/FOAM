/*
 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

*/
/*! 
	@file foam.h
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 20091103 10:32

	@brief This is the main header file for FOAM.

*/

#ifndef __FOAM_H__
#define __FOAM_H__


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#ifndef FOAM_MODITIFG_ALONE
// We must not include sys/time.h if we're only building the itifg module
// This is a little ugly but somehow itifg decided to redefine timeval
#include <sys/time.h>
#endif
#ifndef _GNU_SOURCE				
#define _GNU_SOURCE				// for vasprintf / asprintf
#endif
#include <sys/socket.h>				// networking
#include <arpa/inet.h>				// networking
#ifndef FOAM_MODITIFG_ALONE
// Same as above, itifg redefines u_int32_t
#include <sys/types.h>
#endif
#include <sys/errno.h>
#ifndef FOAM_MODITIFG_ALONE
// Same as above, itifg redefines u_int32_t
#include <stdlib.h>
#endif
#include <syslog.h> 				// used for syslogging
#include <stdarg.h>
#include <pthread.h> 				// threads
#include <stdbool.h> 				// true/false
#include <signal.h> 				// signal handlers
#include <time.h> 					// needed by libevent/event.h
typedef unsigned char u_char;
#include <event.h> 					// include AFTER stdlib.h (which defined u_char needed by event.h)
#include <fcntl.h>
#include <gsl/gsl_linalg.h> 		// this is for SVD / matrix datatype
#include <gsl/gsl_blas.h> 			// this is for SVD

//#include <iostream>
#include <string>
//#include <cstring>


#include "autoconfig.h"
#include "types.h"
#include "protocol.h"

typedef Protocol::Server::Connection Connection;


// PROTOTYPES //
/**************/

// THESE MUST BE DEFINED IN SOME PRIME MODULE!!!!

/*!
 @brief This routine is run at the very beginning of the FOAM program, after configuration has been read.
 
 modInitModule() is one of the cornerstones of the modular design of FOAM. This routine is not defined in
 the framework itself, but must be provided by the prime modules compiled with FOAM. This `hook' provides
 a standardized means to initialize the module before anything has been done, like allocate memory, read in 
 some configuration files, start cameras or anything else.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 @param [in] *cs_config A config_t struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.
 */
int modInitModule(control_t *ptc, config_t *cs_config);

/*!
 @brief This routine is run right after the program has split into two threads.
 
 This routine can be used to initialize things that are not thread safe,
 such as OpenGL. See modInitModule for more details.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 @param [in] *cs_config A config_t struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modPostInitModule(control_t *ptc, config_t *cs_config);

/*!
 @brief This routine is run at the very end of the FOAM program.
 
 modStopModule() can be used to wrap up things related to the module, like stop cameras, set
 filterwheels back or anything else. If this module fails, FOAM *will* exit anyway.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 */
void modStopModule(control_t *ptc);

/*! 
 @brief This routine is run during closed loop.
 
 modClosedLoop() should be provided by a module which does the necessary things in closed loop. Before this
 routine is called in a loop, modClosedInit() is first called once to initialize things related to closed loop
 operation.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedLoop(control_t *ptc);

/*! 
 @brief This routine is run once before entering closed loop.
 
 modClosedInit() should be provided by a module which does the necessary things just before closed loop.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedInit(control_t *ptc);

/*! 
 @brief This routine is run after closed loop.
 
 modClosedFinish() can be used to shut down camera's temporarily, i.e.
 to stop grabbing frames or something similar.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedFinish(control_t *ptc);

/*! 
 @brief This routine is run during open loop.
 
 modOpenLoop() should be provided by a module which does the necessary things in open loop.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modOpenLoop(control_t *ptc);

/*! 
 @brief This routine is run once before entering open loop.
 
 modOpenInit() should be provided by a module which does the necessary things just before open loop.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.  
 */
int modOpenInit(control_t *ptc);

/*! 
 @brief This routine is run after open loop.
 
 modOpenFinish() can be used to shut down camera's temporarily, i.e.
 to stop grabbing frames or something similar.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modOpenFinish(control_t *ptc);

/*! 
 @brief This routine is run in calibration mode.
 
 Slightly different from open and closed mode is the calibration mode. This mode does not have
 a loop which runs forever, but only calls modCalibrate once. It is left to the programmer to decide 
 what to do in this mode. control_t provides a flag (.calmode) to distinguish between different calibration modes.
 
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.  
 */
int modCalibrate(control_t *ptc);

/*!
 @brief This routine is run if a client sends a message over the socket
 
 If the networking thread of the control software receives data, it will 
 split the string received in space-separate words. After that, the
 routine will handle some default commands itself, like 'help', 'quit',
 'shutdown' and others (see parseCmd()). If a command is not recognized, it 
 is passed to this routine, which must then do something, or return 0 to
 indicate an unknown command. parseCmd() itself will then warn the user.\n
 \n
 Besides parsing commands, this routine must also provide help informtion
 about which commands are available in the first place. FOAM itself
 already provides some help on the basic functions, and after sending this
 to the user, modMessage is called with list[0] == 'help' such that this 
 routine can add its own help info.\n
 \n
 If a client sends 'help <topic>', this is also passed to modMessage(),
 with list[0] == help and list[1] == <topic>. It should then give
 information on that topic and return 1, or return 0 if it does not
 'know' that topic.\n
 \n
 For an example of modMessage(), see foam_primemod-dummy.c.
 @param [in] *ptc A control_t struct that has been configured in a prime module 
 @param [in] *client Information on the client that sent data
 @param [in] *list The list of words received over the network
 @param [in] count The amount of words received over the network
 */
int modMessage(control_t *ptc, const client_t *client, char *list[], const int count);

// FOAM (LIBRARY) ROUTINES BEGIN HERE //
/**************************************/

/*!
 @brief This is the routine that is run immediately after threading, and should run modeListen after initializing the prime module.

 */
void *startThread(void *arg);


/*! 
 @brief Runs FOAM in open-loop mode.
 
 Runs the AO system in open loop mode, which is basically a skeleton which 
 calls modOpenInit() once, then enters a loop which runs as long as ptc.mode is 
 set to \c AO_MODE_OPEN and (see aomode_t) calls modOpenLoop() each run. Additionally,
 this function increments ptc.frames each loop. If ptc.mode is unequal to 
 \c AO_MODE_OPEN, this function calls modOpenFinish() and returns to modeListen().
 */
void modeOpen();

/*! 
 @brief Runs the AO closed-loop mode.
 
 Runs the AO system in closed loop mode, which does the same as modeOpen() except
 it calls modClosedInit() first, then modClosedLoop() and finally 
 modClosedFinish(). See modeOpen() for details.
 */
void modeClosed();

/*! 
 @brief Listens to the user and decides what to do next.
 
 This function is called when the system is not in calibration, open loop or
 closed loop-mode. If so, it locks using a pthread condition and waits until
 the networking thread gets a command to do something. When a command is sent
 over the network, the networking thread notices the worker thread which
 is running modeListen() and checks if something needs to be done.
 */
void modeListen();

/*! 
 @brief Runs the AO calibration-loop mode.
 
 This skeleton simply calls modCalibrate() which must be defined in a 
 prime module.
 */
void modeCal();

/*! 
 @brief Listens on a socket for connections
 
 Listen on a socket for connections by any client, for example telnet. Uses 
 the global \a ptc (see control_t) struct to provide data to the connected 
 clients or to change the behaviour of FOAM as dictated by the client.
 
 This function initializes the listening socket, and calls sockAccept() when
 a client connects to it. This event handling is done by libevent, which also
 multiplexes the other socket interactions.
 
 @return \c EXIT_SUCCESS if it ran succesfully, \c EXIT_FAILURE otherwise.
 */
int sockListen();

/*! 
 @brief Accept new client connection.
 
 Accept a new connection pending on \a sock and add the
 socket to the set of active sockets (see conntrack_t and client_t).
 This function is called if there is an event on the main socket, which means 
 someone wants to connect. It spawns a new bufferent event which keeps an eye 
 on activity on the new socket, in which case there is data to be read. 
 
 The event handling for this socket (such as processing incoming user data or
 sending data back to the client) is handled by libevent.
 
 @param [in] sock Socket with pending connection
 @param [in] event The way this function was called
 @param [in] *arg Necessary additional argument to use as caller function for libevent
 */
void sockAccept(int sock, short event, void *arg);

/*!
 @brief Sets a socket to non-blocking mode.
 
 Taken from \c http://unx.ca/log/libevent_echosrv_bufferedc/
 
 @return \c EXIT_SUCCESS if it ran succesfully, \c EXIT_FAILURE otherwise. 
 */
int setNonBlock(int fd);

/*! 
 @brief Process the command given by the user.
 
 This function is called by sockOnRead if there is data on a socket. The data
 is then passed on to this function which interprets it and
 takes action if necessary. Currently this function can only
 read 1 kb in one time maximum (which should be enough).
 
 'msg' contains the string received from the client, which is passed to
 explode() which chops it up in several words.
 
 The first word in 'msg' is compared against a few commands known by parseCmd()
 and if one is found, apropriate actions are taken if necessary. If the command
 is unknown, it is passed to modMessage(), which should be provided by the prime
 module. This way, the commands FOAM accepts can be expanded by the user.
 
 One special case is the 'help' command. If simply 'help' is given with nothing
 after that, parseCmd() calls showHelp() to display general usage information
 for the FOAM framework. After showing this information, modMessage() is also
 called, such that specific usage information for the prime module can also 
 be dispalyed.
 
 If 'help' is followed by another word which is the topic a user requests
 help on, it is passed to showHelp() and modMessage() as well. If these routines
 both return 0, the topic is unknown and an error is given to the user.
  
 @param [in] *msg the char array (max length \c COMMANDLEN)
 @param [in] len the actual length of msg
 @param [in] *client the client_t struct of the client we want to parse a command from
 @return \c EXIT_SUCCESS upon success, or \c EXIT_FAILURE otherwise.
 */
int parseCmd(char *msg, int len, client_t *client);

/*!
 @brief This function is called if there is an error on the socket.
 
 An error on the socket can mean that the client simply wants to disconnect,
 in which cas \c event is equal to \c EVBUFFER_EOF. If so, this is not a problem.
 If \c event is something different, a real error occurred. In both cases, the
 bufferevent is freed, the socket is closed, and the client struct is freed, in that order.
 */
void sockOnErr(struct bufferevent *bev, short event, void *arg);

/*!
 @brief This function is called if there is data to be read on the socket.
 
 Data waiting to be read usually means there is a command coming from one of the connected
 clients. This routine reads the data, checking if it does not exceed \c COMMANDLEN,
 and then passes the result on to parseCmd().
 */
void sockOnRead(struct bufferevent *bev, void *arg);


// TODO: document
static void on_message(Connection *connection, std::string line);
static void on_connect(Connection *connection, bool status);

/*!
 @brief This function is called if the socket is ready for writing.
 
 This function is necessary because passing a NULL pointer to bufferevent_new for the write
 function causes it to crash. Therefore this placeholder is used, which does exactly nothing.
 */
void sockOnWrite(struct bufferevent *bev, void *arg);

/*!
 @brief Open a stream to the appropriate logfiles, as defined in the configuration file.
 
 This function opens the error-, info- and debug-log files *if* they
 are defined in the global struct cs_config (see config_t). If some filenames are equal, the
 logging is linked. This can be useful if users want to log everything to the same
 file. No filename means that no stream will be opened.
 @return EXIT_SUCCESS if the load succeeds, EXIT_FAILURE otherwise.
 */
int initLogFiles();

/*!
 @brief Check & initialize darkfield, flatfield and skyfield files.
 
 Check if the darkfield, flatfield and skyfield files are available, and if so,
 allocate memory for the various images in memory and load them into the
 newly allocated matrices. 
 
 If the files are not present, the FD's will be NULL. Allocate memory anyway in that
 case so we can directly store the frames in the specific memory later on.
 
 If the files are set to NULL, don't use dark/flat/sky field calibration at all.
 */
void checkFieldFiles(wfs_t *wfsinfo);

/*!
 @brief Check if the ptc struct has reasonable values
 
 Check if the values in ptc set by modInitModule() by the user are reasonable.
 If they are not reasonable, give a warning about it. If necessary, allocate
 some memory.
 */
void checkAOConfig(control_t *ptc);

/*!
 @brief Check if the cs_config struct has reasonable values
 
 Check if the values in cs_config set by modInitModule() by the user are reasonable.
 If they are not reasonable, give a warning about it. If necessary, allocate
 some memory.
 */
void checkFOAMConfig(config_t *conf);

/*!
 @brief Give usage information on FOAM to a client.
 
 This function gives help to the cliet which sent a HELP command over the socket.
 Also look at parseCmd() function.
 
 @param [in] *client the client that requested help
 @param [in] *subhelp the helpcode requested by the client (i.e. what topic)
 @return 1 everything went ok, or 0 if the user requested an unknown topic.
 */
int showHelp(const client_t *client, const char *subhelp);

/*!
 @brief Function which wraps up FOAM (free some memory, gives some stats)
 
 This routine does the following:
 \li Print info to user that FOAM is stopping using logInfo()
 \li Run modStopModule() such that modules can stop what they're doing (shutdown cameras and such)
 \li Destroy the mutexes for the threads
 \li Close files for info/error/debug logging.
 */
void stopFOAM();

/*!
 @brief Catches \c SIGINT signals and stops FOAM gracefully, calling \c stopFOAM().
 */
void catchSIGINT(int);

/*!
 @brief This sends a message 'msg' to all connected clients.
 
 @param [in] *msg The message to send to the clients
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int tellClients(char *msg, ...);

/*!
 @brief This sends a message 'msg' to a specific client.
 
 @param [in] *bufev The bufferevent associated with a client
 @param [in] *msg The message to send to the clients
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int tellClient(struct bufferevent *bufev, char *msg, ...);

#endif // __FOAM_H__

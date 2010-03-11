/*
 foam.h -- main FOAM header file. defines FOAM framework 
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
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

#ifndef HAVE_FOAM_H
#define HAVE_FOAM_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#ifndef _GNU_SOURCE				
#define _GNU_SOURCE				// for vasprintf / asprintf
#endif
#include <sys/socket.h>				// networking
#include <arpa/inet.h>				// networking
#include <sys/types.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <syslog.h> 				// used for syslogging
#include <stdarg.h>
#include <pthread.h> 				// threads
#include <signal.h> 				// signal handlers
#include <time.h> 					// needed by libevent/event.h
#include <fcntl.h>
#include <gsl/gsl_linalg.h> 		// this is for SVD / matrix datatype
#include <gsl/gsl_blas.h> 			// this is for SVD

#include <string>
#include <stdexcept>

#include "autoconfig.h"
#include "types.h"
#include "config.h"
#include "protocol.h"
#include "foamctrl.h"

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
 
 @param [in] *ptc A foamctrl struct that has been configured in a prime module
 @param [in] *cs_config A foamcfg struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.
 */
int modInitModule(foamctrl *ptc, foamcfg *cs_config);

/*!
 @brief This routine is run right after the program has split into two threads.
 
 This routine can be used to initialize things that are not thread safe,
 such as OpenGL. See modInitModule for more details.
 
 @param [in] *ptc A foamctrl struct that has been configured in a prime module
 @param [in] *cs_config A foamcfg struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modPostInitModule(foamctrl *ptc, foamcfg *cs_config);

/*!
 @brief This routine is run at the very end of the FOAM program.
 
 modStopModule() can be used to wrap up things related to the module, like stop cameras, set
 filterwheels back or anything else. If this module fails, FOAM *will* exit anyway.
 
 @param [in] *ptc A foamctrl struct that has been configured in a prime module
 */
void modStopModule(foamctrl *ptc);

/*! 
 @brief This routine is run during closed loop.
 
 modClosedLoop() should be provided by a module which does the necessary things in closed loop. Before this
 routine is called in a loop, modClosedInit() is first called once to initialize things related to closed loop
 operation.
 
 @param [in] *ptc A foamctrl struct that has been configured in a prime module
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedLoop(foamctrl *ptc);

/*! 
 @brief This routine is run once before entering closed loop.
 
 modClosedInit() should be provided by a module which does the necessary things just before closed loop.
 @param [in] *ptc A foamctrl struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedInit(foamctrl *ptc);

/*! 
 @brief This routine is run after closed loop.
 
 modClosedFinish() can be used to shut down camera's temporarily, i.e.
 to stop grabbing frames or something similar.
 @param [in] *ptc A foamctrl struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modClosedFinish(foamctrl *ptc);

/*! 
 @brief This routine is run during open loop.
 
 modOpenLoop() should be provided by a module which does the necessary things in open loop.
 @param [in] *ptc A foamctrl struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modOpenLoop(foamctrl *ptc);

/*! 
 @brief This routine is run once before entering open loop.
 
 modOpenInit() should be provided by a module which does the necessary things just before open loop.
 @param [in] *ptc A foamctrl struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.  
 */
int modOpenInit(foamctrl *ptc);

/*! 
 @brief This routine is run after open loop.
 
 modOpenFinish() can be used to shut down camera's temporarily, i.e.
 to stop grabbing frames or something similar.
 
 @param [in] *ptc A foamctrl struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not. 
 */
int modOpenFinish(foamctrl *ptc);

/*! 
 @brief This routine is run in calibration mode.
 
 Slightly different from open and closed mode is the calibration mode. This mode does not have
 a loop which runs forever, but only calls modCalibrate once. It is left to the programmer to decide 
 what to do in this mode. foamctrl provides a flag (.calmode) to distinguish between different calibration modes.
 
 @param [in] *ptc A foamctrl struct that has been configured in a prime module 
 @return EXIT_SUCCESS or EXIT_FAILURE depening on success or not.  
 */
int modCalibrate(foamctrl *ptc);

/*!
 @brief Called when a message is received
 
 If the networking thread of the control software receives data, it will 
 try to handle some default commands itself, like 'help', 'quit',
 'shutdown' and others (see on_message()). If a command is not recognized, it 
 is passed to this routine, which must then do something, or return -1 to
 indicate an unknown command. on_message() itself will then warn the user.

 Besides parsing commands, this routine must also provide help informtion
 about which commands are available in the first place. FOAM itself
 already provides some help on the basic functions, and after sending this
 to the user, modMessage is called with cmd == 'help' such that this 
 routine can add its own help info.

 If a client sends 'help <topic>', this is also passed to modMessage(),
 with cmd == 'help' and the topic is stored in 'line'. It should then give
 information on that topic and return 0, or return -1 if it does not 'know' 
 that topic.
 
 For an example of modMessage(), see foam_primemod-dummy.c.
 
 See also on_message().
 
 @param [in] *ptc A foamctrl struct that has been configured in a prime module 
 @param [in] *connection Reference to the connection
 @param [in] cmd The command given (first word)
 @param [in] line The remainder of the data received.
 */
int modMessage(foamctrl *ptc, Connection *connection, string cmd, string line);

// FOAM (LIBRARY) ROUTINES BEGIN HERE //
/**************************************/

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
 @brief Process commands received.
 
 This function is called when a message is received from a client, which
 is stored in 'line'. This will then be interpreted and appropriate action
 will be taken.
 
 The first word of 'line' is the command, following words can be options or
 parameters. If the command is recognized, it is processed right away. If not,
 it will be passed on to modMessage().
 
 See also showhelp() and modMessage().
 
 @param [in] *connection reference to the connection this applies to.
 @param [in] line the message received
 */
static void on_message(Connection *connection, std::string line);

/*!
 @brief Called when a client (dis)connects to FOAM.
 
 @param [in] *connection reference to the connection this applies to.
 @param [in] status true on connecti, false on disconnect.
*/
static void on_connect(Connection *connection, bool status);

/*!
 @brief Give usage information on FOAM to a client.
 
 This function gives help to the cliet which sent a HELP command over the socket.
 Also look at on_message() function.
 
 @param [in] *connection the client that requested help
 @param [in] topic the helpcode requested by the client (i.e. what topic)
 @param [in] rest the remainder of the query string.
 @return 1 everything went ok, or 0 if the user requested an unknown topic.
 */
static int showhelp(Connection *connection, string topic, string rest);


/*!
 @brief Function which wraps up FOAM (free some memory, gives some stats)
 
 This routine does the following:
 \li Print info to user that FOAM is stopping using logInfo()
 \li Run modStopModule() such that modules can stop what they're doing (shutdown cameras and such)
 \li Destroy the mutexes for the threads
 \li Close files for info/error/debug logging.
 */
int stopFOAM();

/*!
 @brief Catches \c SIGINT signals and stops FOAM gracefully, calling \c stopFOAM().
 */
void catchSIGINT(int);

#endif // HAVE_FOAM_H

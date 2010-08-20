/*
 foam.h -- main FOAM header file, defines FOAM framework 
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
	@brief This is the main header file for FOAM.
*/

#ifndef HAVE_FOAM_H
#define HAVE_FOAM_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#ifndef _GNU_SOURCE				
#define _GNU_SOURCE						// for vasprintf / asprintf
#endif
#include <sys/socket.h>				// networking
#include <arpa/inet.h>				// networking
#include <sys/types.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <syslog.h>						// used for syslogging
#include <stdarg.h>
#include <pthread.h>					// threads
#include <time.h>							// needed by libevent/event.h
#include <fcntl.h>
#include <gsl/gsl_linalg.h> 	// this is for SVD / matrix datatype
#include <gsl/gsl_blas.h> 		// this is for SVD

#include <string>
#include <stdexcept>

#include "autoconfig.h"
#include "types.h"
#include "config.h"
#include "protocol.h"
#include "foamctrl.h"
#include "devices.h"

using namespace std;

typedef Protocol::Server::Connection Connection;

class FOAM {
protected:
	// Properties set at start
	bool nodaemon;											//!< Run daemon or not
	bool error;													//!< Error flag
	string conffile;										//!< Configuration file to use
	string execname;										//!< Executable name, i.e. argv[0]

	struct tm *tm_start;								//!< Start time
	struct tm *tm_end;									//!< End time
	
	struct {
		bool ok;													//!< Track whether a network command is ok or not
	} netio;

	Protocol::Server *protocol;					//!< Network control socket
	
	pthread_mutex_t mode_mutex;					//!< Network thread <-> main thread mutex
	pthread_cond_t mode_cond;
	static pthread_attr_t attr;
	
	/*!
	 @brief Run on new connection to FOAM
	 */
	void on_connect(Connection *connection, bool status);
	/*!
	 @brief Run on new incoming message to FOAM
	 @param [in] *connection connection the message was received on
	 @param [in] line the message received
	 
	 This is called when a new network message is received. Callback registred 
	 through protocol. This routine is virtual and can (and should) be overloaded
	 by the setup-specific child-class such that it can process setup-specific
	 commands in addition to generic commands.
	 */
	virtual void on_message(Connection *connection, std::string line);

	void show_clihelp(bool);						//!< Show help on command-line syntax.
	int show_nethelp(Connection *connection, string topic, string rest); //!< Show help on network command usage
	void show_version();								//!< Show version information
	void show_welcome();								//!< Show welcome banner
	
public:
	FOAM(int argc, char *argv[]);
	virtual ~FOAM() = 0;
	
	foamctrl *ptc;											//!< AO control
	DeviceManager *devices;							//!< Device/hardware management
	Io io;															//!< Screen diagnostics output
	
	bool has_error() { return error; }	//!< Error checking 
	
	int init();													//!< Initialize FOAM setup
	int parse_args(int argc, char *argv[]); //!< Parse command-line arguments
	int load_config();									//!< Load FOAM configuration (from arguments)
	int verify();												//!< Verify setup integrity (from configuration)
	void daemon();											//!< Start network daemon
	int listen();												//!< Start main FOAM control loop
	
	string mode2str(aomode_t m);
	aomode_t str2mode(string m);
	
	/*!
	 @brief Load setup-specific modules
	 
	 This routine should be defined in a child class of FOAM and should load
	 the modules necessary for operation. These typically include a wavefront 
	 sensor with camera and one or more wave front correctors, such as a DM or
	 tip-tilt mirror.
	 */
	virtual int load_modules() = 0;

	int mode_closed();									//!< Closed-loop wrapper, calls child routines.
	/*!
	 @brief Closed-loop initialisation routines
	 TODO: document
	 */
	virtual int closed_init() = 0;
	/*!
	 @brief Closed-loop body routine
	 TODO: document
	 */
	virtual int closed_loop() = 0;
	/*!
	 @brief Closed-loop finalising routine
	 TODO: document
	 */
	virtual int closed_finish() = 0;
	
	int mode_open();										//!< Open-loop wrapper, calls child routines.
	/*!
	 @brief Open-loop initialisation routine
	 TODO: document
	 */
	virtual int open_init() = 0;
	/*!
	 @brief Open-loop body routine
	 TODO: document
	 */
	virtual int open_loop() = 0;
	/*!
	 @brief Open-loop finalising routine
	 TODO: document
	 */
	virtual int open_finish() = 0;
	
	int mode_calib();									//!< Calibration wrapper, calls child routines
	/*!
	 @brief Calibration routine, used to calibrate various system aspects
	 TODO: document
	 */
	virtual int calib() = 0;
};

#endif // HAVE_FOAM_H

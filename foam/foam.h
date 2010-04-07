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

using namespace std;

typedef Protocol::Server::Connection Connection;

class FOAM {
private:
	// Properties set at start
	bool nodaemon;
	bool error;
	string conffile;
	string execname;

	struct tm *tm_start;
	struct tm *tm_end;
		
protected:
	// Network handling class
	Protocol::Server *protocol;
	
	
	// Inter-thread communication (networking- & main thread)
	pthread_mutex_t mode_mutex;
	pthread_cond_t mode_cond;
	static pthread_attr_t attr;
	
	void on_connect(Connection *connection, bool status);
	
	void show_clihelp(bool);
	bool show_nethelp(Connection *connection, string topic, string rest);
	void show_version();
	void show_welcome();
	
public:
	// was: modInitModule(foamctrl *ptc, foamcfg *cs_config);
	FOAM(int argc, char *argv[]);
	// was: modStopModule(foamctrl *ptc);
	virtual ~FOAM() = 0;
	
	// AO control & configuration classes
	foamctrl *ptc;
	foamcfg *cs_config;	
	// Message output and logging 
	Io io;
		
	// Was part of main()
	bool init();
	bool parse_args(int argc, char *argv[]);
	bool load_config();
	bool verify();
	void daemon();
	bool listen();
	
	// was: modInitModule()
	virtual bool load_modules() = 0;

	// was: modMessage();
	virtual void on_message(Connection *connection, std::string line);

	bool mode_closed();
	// was: modClosedInit, Loop, Finish
	virtual bool closed_init() = 0;
	virtual bool closed_loop() = 0;
	virtual bool closed_finish() = 0;
	
	bool mode_open();
	// was: modOpenInit, Loop, Finish
	virtual bool open_init() = 0;
	virtual bool open_loop() = 0;
	virtual bool open_finish() = 0;
	
	bool mode_calib();
	// was: modCalibrate
	virtual bool calib() = 0;
};

#endif // HAVE_FOAM_H
